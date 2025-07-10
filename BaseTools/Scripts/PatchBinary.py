import argparse
import os
import time

class BinaryFilePatcher:
    def __init__(self, OutputFilePath, Patches):
        self.OutputFilePath = OutputFilePath
        self.Patches = Patches

    def ReadBinaryFile(self, FilePath):
        """Read the entire content of a binary file"""
        try:
            with open(FilePath, 'rb') as F:
                return F.read()
        except FileNotFoundError:
            print(f"Error: File not found - {FilePath}")
            return None
        except IOError as E:
            print(f"Error: Could not read file - {FilePath}. {E}")
            return None

    def PatchBinaryFile(self, Offset, Data):
        """Patch the output binary file at the specified offset"""
        try:
            with open(self.OutputFilePath, 'r+b') as F:
                F.seek(Offset)
                F.write(Data)
        except IOError as E:
            print(f"Error: Could not write to file - {self.OutputFilePath}. {E}")

    def UpdateTimestamp(self):
        """Update the timestamp of the output binary file to the current time"""
        CurrentTime = time.time()
        os.utime(self.OutputFilePath, (CurrentTime, CurrentTime))

    def Execute(self):
        """Execute the read and patch operations"""
        # Read the content of the output binary file
        OutputData = self.ReadBinaryFile(self.OutputFilePath)
        if OutputData is None:
            return

        print(f"Read {len(OutputData)} bytes from {self.OutputFilePath}.")

        for Patch in self.Patches:
            InputFilePath, Offset = Patch
            Offset = int(Offset, 0)  # Convert offset to integer
            InputData = self.ReadBinaryFile(InputFilePath)
            if InputData is None:
                continue

            print(f"Read {len(InputData)} bytes from {InputFilePath}.")

            # Check if the input data is different from the output data at the specified offset
            if OutputData[Offset:Offset + len(InputData)] != InputData:
                self.PatchBinaryFile(Offset, InputData)
                print(f"Patched {self.OutputFilePath} at offset {Offset} with data from {InputFilePath}.")
            else:
                # Update the timestamp of the output file
                self.UpdateTimestamp()
                print(f"No changes needed for {self.OutputFilePath} at offset {Offset} with data from {InputFilePath}. Updated timestamp.")

if __name__ == "__main__":
    Parser = argparse.ArgumentParser(description="Patch a binary file with data from other binary files at specified offsets.")
    Parser.add_argument('-o', '--Output', required=True, help="Path to the output binary file")
    Parser.add_argument('Patches', nargs='+', help="List of input files and offsets in the format /path/file:offset")

    Args = Parser.parse_args()

    # Parse the patches
    Patches = []
    for Patch in Args.Patches:
        # Split only on the last colon to handle Windows paths with colons
        Parts = Patch.rsplit(':', 1)
        if len(Parts) == 2:
            Patches.append(Parts)
        else:
            print(f"Error: Invalid patch format - {Patch}")

    Patcher = BinaryFilePatcher(Args.Output, Patches)
    Patcher.Execute()
