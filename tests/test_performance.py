#!/usr/bin/env python3
import subprocess
import time
import sys
import statistics

def run_command(cmd_args):
    """Run a command and return its output and timing"""
    try:
        start_time = time.time()
        result = subprocess.run(cmd_args, capture_output=True, text=True, timeout=10)
        end_time = time.time()
        return result.stdout.strip(), result.stderr.strip(), result.returncode, end_time - start_time
    except subprocess.TimeoutExpired:
        return None, "Command timed out", 1, 10.0

def benchmark_operation(operation_fn, iterations=100, name="operation"):
    """Benchmark an operation multiple times"""
    print(f"Benchmarking {name} ({iterations} iterations)...")
    
    times = []
    for i in range(iterations):
        start_time = time.time()
        try:
            operation_fn(i)
            end_time = time.time()
            times.append(end_time - start_time)
        except Exception as e:
            print(f"Error in iteration {i}: {e}")
            continue
    
    if times:
        avg_time = statistics.mean(times)
        min_time = min(times)
        max_time = max(times)
        median_time = statistics.median(times)
        
        print(f"  Average: {avg_time*1000:.2f}ms")
        print(f"  Median:  {median_time*1000:.2f}ms")
        print(f"  Min:     {min_time*1000:.2f}ms")
        print(f"  Max:     {max_time*1000:.2f}ms")
        print(f"  Ops/sec: {1/avg_time:.0f}")
        
        return avg_time
    else:
        print("  No successful operations!")
        return float('inf')

def test_string_operations_performance():
    """Benchmark basic string operations"""
    print("=== String Operations Performance ===")
    
    def set_operation(i):
        run_command(['../client', 'set', f'perf_key_{i}', f'value_{i}'])
    
    def get_operation(i):
        run_command(['../client', 'get', f'perf_key_{i}'])
    
    def del_operation(i):
        run_command(['../client', 'del', f'perf_key_{i}'])
    
    # First populate some keys for GET/DEL tests
    for i in range(100):
        run_command(['../client', 'set', f'perf_key_{i}', f'value_{i}'])
    
    benchmark_operation(set_operation, 100, "SET operations")
    benchmark_operation(get_operation, 100, "GET operations")
    benchmark_operation(del_operation, 100, "DEL operations")

def test_hash_operations_performance():
    """Benchmark hash operations"""
    print("\n=== Hash Operations Performance ===")
    
    def hset_operation(i):
        run_command(['../client', 'hset', f'hash_perf_{i//10}', f'field_{i}', f'value_{i}'])
    
    def hget_operation(i):
        run_command(['../client', 'hget', f'hash_perf_{i//10}', f'field_{i}'])
    
    def hdel_operation(i):
        run_command(['../client', 'hdel', f'hash_perf_{i//10}', f'field_{i}'])
    
    benchmark_operation(hset_operation, 100, "HSET operations")
    benchmark_operation(hget_operation, 100, "HGET operations")
    benchmark_operation(hdel_operation, 100, "HDEL operations")

def test_sorted_set_performance():
    """Benchmark sorted set operations"""
    print("\n=== Sorted Set Operations Performance ===")
    
    def zadd_operation(i):
        run_command(['../client', 'zadd', f'zset_perf_{i//20}', str(float(i)), f'member_{i}'])
    
    def zscore_operation(i):
        run_command(['../client', 'zscore', f'zset_perf_{i//20}', f'member_{i}'])
    
    def zquery_operation(i):
        run_command(['../client', 'zquery', f'zset_perf_{i//20}', '1', '0', '0', '10'])
    
    def zrem_operation(i):
        run_command(['../client', 'zrem', f'zset_perf_{i//20}', f'member_{i}'])
    
    benchmark_operation(zadd_operation, 100, "ZADD operations")
    benchmark_operation(zscore_operation, 100, "ZSCORE operations")
    benchmark_operation(zquery_operation, 50, "ZQUERY operations")
    benchmark_operation(zrem_operation, 100, "ZREM operations")

def test_expiration_performance():
    """Benchmark expiration operations"""
    print("\n=== Expiration Operations Performance ===")
    
    def expire_operation(i):
        run_command(['../client', 'set', f'expire_perf_{i}', f'value_{i}'])
        run_command(['../client', 'expire', f'expire_perf_{i}', '60'])
    
    def ttl_operation(i):
        run_command(['../client', 'ttl', f'expire_perf_{i}'])
    
    def persist_operation(i):
        run_command(['../client', 'persist', f'expire_perf_{i}'])
    
    benchmark_operation(expire_operation, 50, "SET+EXPIRE operations")
    benchmark_operation(ttl_operation, 100, "TTL operations")
    benchmark_operation(persist_operation, 50, "PERSIST operations")

def test_large_data_performance():
    """Test performance with larger data sizes"""
    print("\n=== Large Data Performance ===")
    
    # Test with different value sizes
    sizes = [100, 1000, 10000]
    
    for size in sizes:
        large_value = 'x' * size
        
        def large_set_operation(i):
            run_command(['../client', 'set', f'large_{size}_{i}', large_value])
        
        def large_get_operation(i):
            run_command(['../client', 'get', f'large_{size}_{i}'])
        
        print(f"\n--- {size} byte values ---")
        benchmark_operation(large_set_operation, 20, f"SET {size}B")
        benchmark_operation(large_get_operation, 20, f"GET {size}B")

def test_concurrent_load():
    """Test basic load with multiple rapid operations"""
    print("\n=== Concurrent Load Test ===")
    
    start_time = time.time()
    operations = 0
    
    # Rapid fire operations for 5 seconds
    end_time = start_time + 5.0
    
    while time.time() < end_time:
        key = f"load_test_{operations}"
        
        # SET
        stdout, stderr, code, duration = run_command(['../client', 'set', key, f'value_{operations}'])
        if code == 0:
            operations += 1
        
        # GET
        stdout, stderr, code, duration = run_command(['../client', 'get', key])
        if code == 0:
            operations += 1
        
        # Check if we should continue
        if time.time() >= end_time:
            break
    
    total_time = time.time() - start_time
    ops_per_second = operations / total_time
    
    print(f"  Total operations: {operations}")
    print(f"  Total time: {total_time:.2f}s")
    print(f"  Operations/second: {ops_per_second:.0f}")

def test_memory_stress():
    """Test server behavior under memory stress"""
    print("\n=== Memory Stress Test ===")
    
    print("Creating many keys to test memory usage...")
    
    start_time = time.time()
    successful_sets = 0
    
    # Try to create 10,000 keys
    for i in range(10000):
        stdout, stderr, code, duration = run_command(['../client', 'set', f'stress_{i}', f'data_{i}_{"x"*50}'])
        if code == 0:
            successful_sets += 1
        
        # Print progress every 1000 operations
        if i % 1000 == 0:
            print(f"  Progress: {i}/10000 keys created")
    
    creation_time = time.time() - start_time
    
    print(f"  Successfully created: {successful_sets} keys")
    print(f"  Creation time: {creation_time:.2f}s")
    print(f"  Creation rate: {successful_sets/creation_time:.0f} keys/sec")
    
    # Test retrieval performance with many keys
    print("Testing retrieval with many keys...")
    
    def stress_get_operation(i):
        run_command(['../client', 'get', f'stress_{i*10}'])  # Get every 10th key
    
    benchmark_operation(stress_get_operation, 100, "GET with 10k keys")
    
    # Cleanup some keys
    print("Cleaning up stress test keys...")
    for i in range(0, successful_sets, 10):  # Delete every 10th key
        run_command(['../client', 'del', f'stress_{i}'])

def main():
    print("Redis Performance Tests")
    print("Make sure the Redis server is running!")
    print("=" * 50)
    
    try:
        test_string_operations_performance()
        test_hash_operations_performance()
        test_sorted_set_performance()
        test_expiration_performance()
        test_large_data_performance()
        test_concurrent_load()
        test_memory_stress()
        
        print("\n" + "=" * 50)
        print("Performance testing completed!")
        print("Note: Performance will vary based on system resources")
        
    except Exception as e:
        print(f"\nPerformance test failed: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nPerformance test interrupted by user")
        sys.exit(1)

if __name__ == "__main__":
    main()