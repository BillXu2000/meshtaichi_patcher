import subprocess

subprocess.run('git pull', shell=True)
subprocess.run('rm -r dist', shell=True)

for i in range(8, 11):
    cmd = f'zsh -c "source ~/.zshrc; conda activate py3{i}; python3.{i} setup.py sdist bdist_wheel"'
    # print(cmd)
    subprocess.run(cmd, shell=True)
