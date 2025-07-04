import subprocess

cases = """
    $ ../client hget janis height
    (null)
    $ ../client hset janis height 180
    (null)
    $ ../client hget janis height
    (str) 180
    $ ../client hdel janis height
    (null)
    $ ../client hget janis height
    (null)
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