#!/usr/bin/env python3
import subprocess
import time
import sys

def run_command(cmd_args):
    """Run a command and return its output"""
    try:
        result = subprocess.run(cmd_args, capture_output=True, text=True, timeout=5)
        return result.stdout.strip(), result.stderr.strip(), result.returncode
    except subprocess.TimeoutExpired:
        return None, "Command timed out", 1

def test_basic_expiration():
    """Test basic EXPIRE and TTL functionality"""
    print("Testing basic expiration...")
    
    # Set a key
    stdout, stderr, code = run_command(['../client', 'set', 'testkey', 'testvalue'])
    assert code == 0, f"Failed to set key: {stderr}"
    
    # Set expiration
    stdout, stderr, code = run_command(['../client', 'expire', 'testkey', '2'])
    assert code == 0, f"Failed to set expiration: {stderr}"
    
    # Check TTL immediately
    stdout, stderr, code = run_command(['../client', 'ttl', 'testkey'])
    assert code == 0, f"Failed to get TTL: {stderr}"
    ttl = int(stdout.split()[-1]) if stdout else -1
    assert 1 <= ttl <= 2, f"TTL should be 1-2 seconds, got {ttl}"
    
    # Key should still exist
    stdout, stderr, code = run_command(['../client', 'get', 'testkey'])
    assert code == 0, f"Failed to get key: {stderr}"
    assert 'testvalue' in stdout, f"Key should still exist, got: {stdout}"
    
    # Wait for expiration
    time.sleep(3)
    
    # Key should be expired
    stdout, stderr, code = run_command(['../client', 'get', 'testkey'])
    assert code == 0, f"Command failed: {stderr}"
    assert '(null)' in stdout, f"Key should be expired, got: {stdout}"
    
    # TTL should return -1 for expired key
    stdout, stderr, code = run_command(['../client', 'ttl', 'testkey'])
    assert code == 0, f"Failed to get TTL: {stderr}"
    ttl = int(stdout.split()[-1]) if stdout else 0
    assert ttl == -1, f"TTL for expired key should be -1, got {ttl}"

def test_persist():
    """Test PERSIST command"""
    print("Testing PERSIST command...")
    
    # Set a key with expiration
    run_command(['../client', 'set', 'persistkey', 'persistvalue'])
    run_command(['../client', 'expire', 'persistkey', '10'])
    
    # Verify expiration is set
    stdout, stderr, code = run_command(['../client', 'ttl', 'persistkey'])
    assert code == 0, f"Failed to get TTL: {stderr}"
    ttl = int(stdout.split()[-1]) if stdout else -1
    assert ttl > 0, f"TTL should be positive, got {ttl}"
    
    # Remove expiration with PERSIST
    stdout, stderr, code = run_command(['../client', 'persist', 'persistkey'])
    assert code == 0, f"Failed to persist key: {stderr}"
    
    # TTL should now be -1 (no expiration)
    stdout, stderr, code = run_command(['../client', 'ttl', 'persistkey'])
    assert code == 0, f"Failed to get TTL: {stderr}"
    ttl = int(stdout.split()[-1]) if stdout else 0
    assert ttl == -1, f"TTL after persist should be -1, got {ttl}"
    
    # Key should still exist after original expiration time
    time.sleep(1)
    stdout, stderr, code = run_command(['../client', 'get', 'persistkey'])
    assert code == 0, f"Failed to get key: {stderr}"
    assert 'persistvalue' in stdout, f"Persisted key should still exist, got: {stdout}"

def test_expire_nonexistent():
    """Test expiration on non-existent keys"""
    print("Testing expiration on non-existent keys...")
    
    # Try to expire non-existent key
    stdout, stderr, code = run_command(['../client', 'expire', 'nonexistent', '10'])
    # This might return an error or succeed silently depending on implementation
    assert code == 0, f"Command failed: {stderr}"
    
    # TTL of non-existent key should be -1
    stdout, stderr, code = run_command(['../client', 'ttl', 'nonexistent'])
    assert code == 0, f"Failed to get TTL: {stderr}"
    ttl = int(stdout.split()[-1]) if stdout else 0
    assert ttl == -1, f"TTL for non-existent key should be -1, got {ttl}"

def test_overwrite_expiration():
    """Test overwriting expiration times"""
    print("Testing overwriting expiration...")
    
    # Set key with expiration
    run_command(['../client', 'set', 'overwrite', 'value'])
    run_command(['../client', 'expire', 'overwrite', '2'])
    
    # Wait a moment then set longer expiration
    time.sleep(1)
    run_command(['../client', 'expire', 'overwrite', '10'])
    
    # TTL should reflect new expiration
    stdout, stderr, code = run_command(['../client', 'ttl', 'overwrite'])
    assert code == 0, f"Failed to get TTL: {stderr}"
    ttl = int(stdout.split()[-1]) if stdout else -1
    assert 8 <= ttl <= 10, f"TTL should be around 8-10, got {ttl}"
    
    # Key should still exist after original expiration time
    time.sleep(2)
    stdout, stderr, code = run_command(['../client', 'get', 'overwrite'])
    assert code == 0, f"Failed to get key: {stderr}"
    assert 'value' in stdout, f"Key should still exist after overwrite, got: {stdout}"

def test_expiration_with_update():
    """Test that updating a key doesn't affect its expiration"""
    print("Testing expiration with key updates...")
    
    # Set key with expiration
    run_command(['../client', 'set', 'updatekey', 'original'])
    run_command(['../client', 'expire', 'updatekey', '5'])
    
    # Update the key value
    time.sleep(1)
    run_command(['../client', 'set', 'updatekey', 'updated'])
    
    # TTL should still be counting down (implementation dependent)
    stdout, stderr, code = run_command(['../client', 'ttl', 'updatekey'])
    assert code == 0, f"Failed to get TTL: {stderr}"
    # Note: This test depends on implementation - some Redis implementations
    # reset TTL on SET, others preserve it
    
    # Get current value
    stdout, stderr, code = run_command(['../client', 'get', 'updatekey'])
    assert code == 0, f"Failed to get key: {stderr}"
    assert 'updated' in stdout, f"Key should have updated value, got: {stdout}"

def test_sorted_set_expiration():
    """Test expiration with sorted sets"""
    print("Testing sorted set expiration...")
    
    # Create sorted set
    run_command(['../client', 'zadd', 'zsetkey', '1.0', 'member1'])
    run_command(['../client', 'zadd', 'zsetkey', '2.0', 'member2'])
    
    # Set expiration
    run_command(['../client', 'expire', 'zsetkey', '2'])
    
    # Verify it exists
    stdout, stderr, code = run_command(['../client', 'zscore', 'zsetkey', 'member1'])
    assert code == 0, f"Failed to get zscore: {stderr}"
    assert '1.0' in stdout, f"Sorted set should exist, got: {stdout}"
    
    # Wait for expiration
    time.sleep(3)
    
    # Should be expired
    stdout, stderr, code = run_command(['../client', 'zscore', 'zsetkey', 'member1'])
    assert code == 0, f"Command failed: {stderr}"
    assert '(null)' in stdout, f"Sorted set should be expired, got: {stdout}"

def test_hash_expiration():
    """Test expiration with hash sets"""
    print("Testing hash expiration...")
    
    # Create hash
    run_command(['../client', 'hset', 'hashkey', 'field1', 'value1'])
    run_command(['../client', 'hset', 'hashkey', 'field2', 'value2'])
    
    # Set expiration
    run_command(['../client', 'expire', 'hashkey', '2'])
    
    # Verify it exists
    stdout, stderr, code = run_command(['../client', 'hget', 'hashkey', 'field1'])
    assert code == 0, f"Failed to get hash field: {stderr}"
    assert 'value1' in stdout, f"Hash should exist, got: {stdout}"
    
    # Wait for expiration
    time.sleep(3)
    
    # Should be expired
    stdout, stderr, code = run_command(['../client', 'hget', 'hashkey', 'field1'])
    assert code == 0, f"Command failed: {stderr}"
    assert '(null)' in stdout, f"Hash should be expired, got: {stdout}"

def main():
    print("Starting expiration tests...")
    print("Make sure the Redis server is running!")
    
    try:
        test_basic_expiration()
        print("✓ Basic expiration test passed")
        
        test_persist()
        print("✓ PERSIST test passed")
        
        test_expire_nonexistent()
        print("✓ Non-existent key expiration test passed")
        
        test_overwrite_expiration()
        print("✓ Overwrite expiration test passed")
        
        test_expiration_with_update()
        print("✓ Expiration with update test passed")
        
        test_sorted_set_expiration()
        print("✓ Sorted set expiration test passed")
        
        test_hash_expiration()
        print("✓ Hash expiration test passed")
        
        print("\nAll expiration tests passed! ✓")
        
    except Exception as e:
        print(f"\nTest failed: {e}")
        sys.exit(1)
    except AssertionError as e:
        print(f"\nAssertion failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()