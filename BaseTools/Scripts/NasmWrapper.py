import sys
import subprocess

def Main():
    NasmPath = sys.argv[1]
    Args = sys.argv[2:]

    RspFile = None
    RspIndex = None
    for I, Arg in enumerate(Args):
        if Arg.startswith('@'):
            RspFile = Arg[1:]
            RspIndex = I
            break

    if RspFile:
        with open(RspFile, 'r') as F:
            RspArgs = F.read().split()
        Args = Args[:RspIndex] + RspArgs + Args[RspIndex+1:]

    Result = subprocess.run([NasmPath] + Args)
    sys.exit(Result.returncode)

if __name__ == '__main__':
    Main()
