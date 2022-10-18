import subprocess

subprocess.run('rm dist/* -y')
subprocess.run('git pull')

for i in range(8, 11):
    cmd = f'zsh -c "source ~/.zshrc; conda activate py3{i}; python3.{i} setup.py sdist bdist_wheel"'
    # print(cmd)
    subprocess.run(cmd, shell=True)
