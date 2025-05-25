#include <stdint.h>
#include <random>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <thread>
#include <mutex>
#include <functional>
#include "Math.h"
#include <shared_mutex>


/* Our trading system contains a number of calculation engines that perform calcs on a request basis.
 * The goal of this assignment is to come up with a generic framework for accelerating the calculations
 * in these engines. We have given you a benchmark test that provides a workload that is representative
 * of the types of calculations we are trying to accelerate.
 *
 * The goal of this assignment is to get the provided test to run as quickly as possible.
 *
 * Your solution should be generic. It should be easily extendible to methods other than IsPrime. It should
 * also be extendible to calculations with various function signatures. You should not modify IsPrime or Math.h.
*/

#if false
Thread
* add threads for calculation: divide tasks into n parts
* (Further improvement): use thread pool, to avoid thread creation overhead

Cache
* add a cache for results. (for future memory improvement, consider std::bitset)
* use a thread local cache to avoid read lock

Lock:
* used shared_mutex for cache to allow multiple threads to read simultaneously
* Sharding: use a vector of unordered_map for cache, where each map corresponds to a shard based on the input value, to avoid lock the whole cache
* avoid locking the cache for every single access, instead lock it only when necessary: Compute primes after releasing the shared lock but before acquiring the exclusive lock
* use a power of 2 for SHARDING_SIZE to allow bitmasking for faster shard indexing

Data structure:
* (Further improvement): use lock free data structures where possible, such as concurrent queues or lock - free hash maps
* use std::vector for inputs and outputs, as it is more cache friendly than std::list or std::deque
* for all vector / unordered_map, reserve a size to avoid reallocations, use emplace_back rather than push_back to avoid copy / move
* (Further improvement) : use vector to replace unordered_map for cache, if the input range is known and small enough, to reduce memory overhead and improve cache locality

Cache friendly
* only use if statement once, avoid branch prediction issues
* align data structures to cache line size(64 bytes) to avoid false sharing
* inline functions to reduce function call overhead
* (Further improvement) Prefetch cache entries: __builtin_prefetch(&isPrimeCache[next_index]);
#endif

// power of 2, use bitmasking (input & (SHARDING_SIZE - 1))  for faster shard indexing
constexpr size_t SHARDING_SIZE = 128;

inline int64_t getIndex(int64_t input)
{
	return (input & (SHARDING_SIZE - 1));
}

static std::vector<int64_t> DoCalc(std::vector<int64_t>& inputs, std::function<double(int64_t)> calc)
{
	static std::vector<std::unordered_map<int64_t, bool>> isPrimeCache(SHARDING_SIZE, std::unordered_map<int64_t, bool>(inputs.size()/SHARDING_SIZE  + 1));
	
	struct alignas(64) PaddedMutex {
		std::shared_mutex mtx;
	};
	static std::vector<PaddedMutex> cacheMutex(SHARDING_SIZE);

	size_t threadNum = std::thread::hardware_concurrency();
	if (!threadNum)
		threadNum = 4;

	std::vector<int64_t> outputs(inputs.size());
	size_t chunk = (inputs.size() + threadNum - 1) / threadNum;
	std::vector<std::thread> threads;

	auto worker = [&](size_t start, size_t end)
		{
			thread_local std::unordered_map<int64_t, bool> localCache(inputs.size());
			
			for (size_t i = start; i < end; ++i)
			{
				int64_t input = inputs[i];

				// Check local cache first
				auto it = localCache.find(input); 
				if (it != localCache.end()) 
				{
					outputs[i] = it->second;
					continue;
				}

				// Try to read from global cache with shared lock
				int shard = getIndex(input); // Shard the cache based on input value
				{
					std::shared_lock<std::shared_mutex> lock(cacheMutex[shard].mtx);
					auto it = isPrimeCache[shard].find(input);
					if (it != isPrimeCache[shard].end()) {
						outputs[i] = it->second;
						continue;
					}
				}

				// Compute and store in cache with exclusive lock
				bool isPrime = calc(input);
				localCache[input] = isPrime; // Store in thread-local cache
				{
					std::unique_lock<std::shared_mutex> lock(cacheMutex[shard].mtx);
					isPrimeCache[shard][input] = isPrime;
				}
				outputs[i] = isPrime;
			}
		};

	for (size_t i = 0; i < threadNum; ++i)
	{
		size_t start = i * chunk;
		size_t end = std::min(start + chunk, inputs.size());
		if (start < end) // Ensure we don't create threads for empty ranges
		{
			threads.emplace_back(worker, start, end);
		}
	}

	for (auto& thread : threads)
	{
		thread.join();
	}
	
	return outputs;
}

static std::vector<int64_t> DoCalc(const std::vector<int64_t>& inputs)
{
	std::vector<int64_t> outputs;
	outputs.reserve(inputs.size());
	for (auto input : inputs)
	{
		outputs.push_back(MathLibrary::Math::IsPrime(input));
	}

	return outputs;
}

int main(int argc, char * argv[])
{
  std::default_random_engine generator;
  std::uniform_int_distribution<int64_t> distribution(1, 1000000000000);
  std::vector<int64_t> inputs;
  for(int32_t i = 0; i < 500000; ++i)
  {
    inputs.emplace_back(distribution(generator));
  }
  auto tick = std::chrono::high_resolution_clock::now();
  std::vector<int64_t> outputs = DoCalc(inputs, MathLibrary::Math::IsPrime);
  auto tock = std::chrono::high_resolution_clock::now();
  std::cout << "finished: " << (std::chrono::duration_cast<std::chrono::duration<double>>(tock - tick)).count() << std::endl;
  return 0;
};
