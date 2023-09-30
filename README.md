# README
## Building

Setup the EDK thing, see the instructions in the EDK II Repo.

## Testing

Testing requires `cmake` to execute

```bash
cd ./HackingGame/HackingGame/
mkdir -p build
cd build
cmake ..
cmake --build . -j
./test
```
