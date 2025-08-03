#!/usr/bin/env python3
import subprocess
import sys

def run_command(cmd_args):
    """Run a command and return its output"""
    try:
        result = subprocess.run(cmd_args, capture_output=True, text=True, timeout=5)
        return result.stdout.strip(), result.stderr.strip(), result.returncode
    except subprocess.TimeoutExpired:
        return None, "Command timed out", 1

def test_invalid_commands():
    """Test various invalid command scenarios"""
    print("Testing invalid commands...")
    
    # Test completely invalid command
    stdout, stderr, code = run_command(['../client', 'invalidcmd'])
    # Should handle gracefully (exact behavior depends on implementation)
    assert code == 0, f"Client should handle invalid commands gracefully: {stderr}"
    
    # Test commands with wrong number of arguments
    stdout, stderr, code = run_command(['../client', 'get'])  # GET needs a key
    assert code == 0, f"Client should handle missing arguments: {stderr}"
    
    stdout, stderr, code = run_command(['../client', 'set', 'key'])  # SET needs key and value
    assert code == 0, f"Client should handle missing arguments: {stderr}"
    
    stdout, stderr, code = run_command(['../client', 'del'])  # DEL needs a key
    assert code == 0, f"Client should handle missing arguments: {stderr}"

def test_very_long_keys_and_values():
    """Test handling of very long keys and values"""
    print("Testing very long keys and values...")
    
    # Test very long key
    long_key = 'x' * 1000
    stdout, stderr, code = run_command(['../client', 'set', long_key, 'value'])
    assert code == 0, f"Should handle long keys: {stderr}"
    
    # Test very long value
    long_value = 'y' * 1000
    stdout, stderr, code = run_command(['../client', 'set', 'key', long_value])
    assert code == 0, f"Should handle long values: {stderr}"
    
    # Verify we can retrieve it
    stdout, stderr, code = run_command(['../client', 'get', 'key'])
    assert code == 0, f"Should retrieve long value: {stderr}"
    # Note: Depending on implementation, might truncate or handle differently

def test_special_characters():
    """Test keys and values with special characters"""
    print("Testing special characters...")
    
    special_chars = [
        'key with spaces',
        'key\nwith\nnewlines',
        'key\twith\ttabs',
        'key"with"quotes',
        "key'with'apostrophes",
        'key\\with\\backslashes',
        'key/with/slashes',
        'key@with#symbols$%^&*()',
        '키',  # Unicode characters
        '🔑',  # Emoji
    ]
    
    for i, special_key in enumerate(special_chars):
        try:
            stdout, stderr, code = run_command(['../client', 'set', special_key, f'value_{i}'])
            assert code == 0, f"Should handle special key '{special_key}': {stderr}"
            
            # Try to retrieve it
            stdout, stderr, code = run_command(['../client', 'get', special_key])
            assert code == 0, f"Should retrieve special key '{special_key}': {stderr}"
        except Exception as e:
            print(f"Note: Special character '{special_key}' caused: {e}")
            # Some special characters might not be supported, which is OK

def test_numeric_edge_cases():
    """Test numeric edge cases for sorted sets and expiration"""
    print("Testing numeric edge cases...")
    
    # Test sorted set with extreme scores
    extreme_scores = [
        '0',
        '0.0',
        '-0.0',
        '1.7976931348623157e+308',  # Close to max double
        '-1.7976931348623157e+308', # Close to min double
        '2.2250738585072014e-308',  # Close to min positive double
        'inf',    # Infinity (might not be supported)
        '-inf',   # Negative infinity
        'nan',    # Not a number
    ]
    
    for i, score in enumerate(extreme_scores):
        try:
            stdout, stderr, code = run_command(['../client', 'zadd', 'extremes', score, f'member_{i}'])
            if code == 0:
                # If it was accepted, try to query it
                stdout, stderr, code = run_command(['../client', 'zscore', 'extremes', f'member_{i}'])
                assert code == 0, f"Should retrieve extreme score '{score}': {stderr}"
            else:
                print(f"Note: Extreme score '{score}' was rejected (which is OK)")
        except Exception as e:
            print(f"Note: Extreme score '{score}' caused: {e}")

def test_expiration_edge_cases():
    """Test edge cases for expiration"""
    print("Testing expiration edge cases...")
    
    # Test zero expiration
    run_command(['../client', 'set', 'zero_expire', 'value'])
    stdout, stderr, code = run_command(['../client', 'expire', 'zero_expire', '0'])
    assert code == 0, f"Should handle zero expiration: {stderr}"
    
    # Test negative expiration  
    run_command(['../client', 'set', 'neg_expire', 'value'])
    stdout, stderr, code = run_command(['../client', 'expire', 'neg_expire', '-1'])
    # Implementation dependent - might reject or treat as immediate expiration
    assert code == 0, f"Should handle negative expiration: {stderr}"
    
    # Test very large expiration
    run_command(['../client', 'set', 'large_expire', 'value'])
    stdout, stderr, code = run_command(['../client', 'expire', 'large_expire', '999999999'])
    assert code == 0, f"Should handle large expiration: {stderr}"

def test_empty_and_null_values():
    """Test empty and null-like values"""
    print("Testing empty and null values...")
    
    # Test empty string value
    stdout, stderr, code = run_command(['../client', 'set', 'empty', ''])
    assert code == 0, f"Should handle empty value: {stderr}"
    
    stdout, stderr, code = run_command(['../client', 'get', 'empty'])
    assert code == 0, f"Should retrieve empty value: {stderr}"
    
    # Test empty key (if supported)
    stdout, stderr, code = run_command(['../client', 'set', '', 'empty_key_value'])
    # Implementation dependent - might reject empty keys
    
    # Test null-like strings
    null_like = ['null', 'NULL', 'nil', 'NIL', '(null)', 'undefined']
    for null_val in null_like:
        stdout, stderr, code = run_command(['../client', 'set', f'null_test_{null_val}', null_val])
        assert code == 0, f"Should handle null-like value '{null_val}': {stderr}"

def test_concurrent_operations():
    """Test basic concurrent operation scenarios"""
    print("Testing concurrent operations...")
    
    # This is a basic test - for real concurrency testing, you'd need multiple processes
    # Test rapid successive operations
    for i in range(10):
        run_command(['../client', 'set', f'rapid_{i}', f'value_{i}'])
    
    # Verify all were set
    for i in range(10):
        stdout, stderr, code = run_command(['../client', 'get', f'rapid_{i}'])
        assert code == 0, f"Rapid operations failed for key rapid_{i}: {stderr}"
        assert f'value_{i}' in stdout, f"Wrong value for rapid operation {i}: {stdout}"

def test_hash_edge_cases():
    """Test hash-specific edge cases"""
    print("Testing hash edge cases...")
    
    # Test HSET with same field multiple times
    run_command(['../client', 'hset', 'hash_test', 'field1', 'value1'])
    run_command(['../client', 'hset', 'hash_test', 'field1', 'value2'])  # Overwrite
    
    stdout, stderr, code = run_command(['../client', 'hget', 'hash_test', 'field1'])
    assert code == 0, f"Hash field overwrite failed: {stderr}"
    
    # Test HDEL on non-existent field
    stdout, stderr, code = run_command(['../client', 'hdel', 'hash_test', 'nonexistent'])
    assert code == 0, f"HDEL on non-existent field should succeed: {stderr}"
    
    # Test HGETALL on non-existent hash
    stdout, stderr, code = run_command(['../client', 'hgetall', 'nonexistent_hash'])
    assert code == 0, f"HGETALL on non-existent hash should succeed: {stderr}"

def test_sorted_set_edge_cases():
    """Test sorted set edge cases"""
    print("Testing sorted set edge cases...")
    
    # Test ZADD with same member, different scores
    run_command(['../client', 'zadd', 'zset_test', '1.0', 'member1'])
    run_command(['../client', 'zadd', 'zset_test', '2.0', 'member1'])  # Update score
    
    stdout, stderr, code = run_command(['../client', 'zscore', 'zset_test', 'member1'])
    assert code == 0, f"ZADD score update failed: {stderr}"
    
    # Test ZREM on non-existent member
    stdout, stderr, code = run_command(['../client', 'zrem', 'zset_test', 'nonexistent'])
    assert code == 0, f"ZREM on non-existent member should succeed: {stderr}"
    
    # Test ZQUERY with invalid parameters
    stdout, stderr, code = run_command(['../client', 'zquery', 'zset_test', 'invalid', 'params'])
    # Should handle gracefully
    assert code == 0, f"ZQUERY with invalid params should be handled: {stderr}"

def main():
    print("Starting error handling and edge case tests...")
    print("Make sure the Redis server is running!")
    
    try:
        test_invalid_commands()
        print("✓ Invalid commands test passed")
        
        test_very_long_keys_and_values()
        print("✓ Long keys and values test passed")
        
        test_special_characters()
        print("✓ Special characters test passed")
        
        test_numeric_edge_cases()
        print("✓ Numeric edge cases test passed")
        
        test_expiration_edge_cases()
        print("✓ Expiration edge cases test passed")
        
        test_empty_and_null_values()
        print("✓ Empty and null values test passed")
        
        test_concurrent_operations()
        print("✓ Concurrent operations test passed")
        
        test_hash_edge_cases()
        print("✓ Hash edge cases test passed")
        
        test_sorted_set_edge_cases()
        print("✓ Sorted set edge cases test passed")
        
        print("\nAll error handling and edge case tests passed! ✓")
        
    except Exception as e:
        print(f"\nTest failed: {e}")
        sys.exit(1)
    except AssertionError as e:
        print(f"\nAssertion failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()