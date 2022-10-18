import subprocess

subprocess.run('rm dist/* -y')

for i in range(7, 11):
    cmd = f'zsh -c "source ~/.zshrc; conda activate py3{i}; python3 setup.py sdist bdist_wheel -p manylinux_2_27_x86_64"'
    # print(cmd)
    subprocess.run(cmd, shell=True)

subprocess.run('ssh m1 "cd bx2k/meshtaichi_patcher; python3 build_m1.py"', shell=True)
subprocess.run('scp -r m1:~/bx2k/meshtaichi_patcher/dist .', shell=True)
