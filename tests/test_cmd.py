import subprocess

cases = """
    $ ../client get janis
    (null)
    $ ../client set janis labakais
    (null)
    $ ../client get janis
    (str) labakais
    $ ../client del janis
    (null)
    $ ../client get janis
    (null)
    $ ../client zscore asdf n1
    (null)
    $ ../client zquery xxx 1 asdf 1 10
    (arr) len=0
    $ ../client zadd zset 1 n1
    (int) 1
    $ ../client zadd zset 2 n2
    (int) 1
    $ ../client zadd zset 1.1 n1
    (int) 0
    $ ../client zscore zset n1
    (double) 1.100000
    $ ../client zquery zset 1 "" 0 10
    (arr) len=4
    (double) 1.100000
    (str) n1
    (double) 2.000000
    (str) n2
    $ ../client zquery zset 1.1 "" 1 10
    (arr) len=2
    (double) 2.000000
    (str) n2
    $ ../client zquery zset 1.1 "" 2 10
    (arr) len=0
    $ ../client zrem zset adsf
    (int) 0
    $ ../client zrem zset n1
    (int) 1
    $ ../client zquery zset 1 "" 0 10
    (arr) len=2
    (double) 2.000000
    (str) n2
"""

cmds = [c.strip().split(' ')[1:] for c in cases.split('\n') if c and c.strip()[0] == '$']
outputs = []

curr_out = []
for i, line in enumerate(cases.split('\n')):
    line = line.strip()
    if line and line[0] == '$':
        if len(curr_out) != 0:
            outputs.append('\n'.join(curr_out))
        curr_out = []
    else:
        curr_out.append(line)
if len(curr_out):
    outputs.append('\n'.join(curr_out))


for cmd, out in zip(cmds, outputs[1:]):
    cmd_out = subprocess.run(cmd, capture_output=True)

    try:
        assert cmd_out.stdout.decode('utf-8').strip() == out.strip()
    except AssertionError:
        print(cmd)
        print(f"output:\n{cmd_out.stdout.decode('utf-8').strip()}\n")
        print(f"goal:\n{out}\n\n")
        continue