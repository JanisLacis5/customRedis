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
    $ ../client hset janis age 18
    (null)
    $ ../client hset janis height 200
    (null)
    $ ../client hset janis school 'riga state gymnasium no2'
    (null)
    $ ../client hget janis age
    (str) 18
    $ ../client hget janis school
    (str) 'riga state gymnasium no2'
    $ ../client hdel janis age
    (null)
    $ ../client hget janis age
    (null)
    $ ../client hgetall janis
    (arr) len=2
    (str) school
    (str) height
    $ ../client hset janis age 20
    (null)
    $ ../client hgetall janis
    (arr) len=3
    (str) age
    (str) school
    (str) height
"""

tmp_cmds = [c.strip().split(' ')[1:] for c in cases.split('\n') if c and c.strip()[0] == '$']
outputs = []
cmds = []

for cmd in tmp_cmds:
    u = []
    start = ""
    add = False
    for token in cmd:
        if add:
            start += " " + token
        elif token[0] == "'" or token[0] == '"':
            start += token
            add = True
        elif token[-1] == "'" or token[-1] == '"':
            start += token
            add = False
            u.append(start)
            start = ""
        else:
            u.append(token)
    if start != "":
        u.append(start)
    cmds.append(u)


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