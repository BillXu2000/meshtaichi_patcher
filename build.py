import subprocess

for i in range(7, 11):
    cmd = f'zsh -c "source ~/.zshrc; conda activate py3{i}; python3 setup.py sdist bdist_wheel -p manylinux_2_27_x86_64"'
    # print(cmd)
    subprocess.run(cmd, shell=True)
