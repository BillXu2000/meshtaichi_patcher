import subprocess

while True:
    subprocess.run("python3 ./example/test.py", shell=True, check=True)
    # subprocess.run("exit 0", shell=True, check=True)
