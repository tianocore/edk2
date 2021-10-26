HDA_CONTENTS=hda-contents
if [ ! -d "${HDA_CONTENTS}" ]; then
  mkdir ${HDA_CONTENTS}
fi

qemu-system-x86_64 \
  -nodefaults \
  -machine q35 \
  -smp 4 \
  -m 128 \
  -serial mon:stdio \
  -vga std \
  --net none \
  -drive if=pflash,format=raw,file=Build/OvmfX64/DEBUG_GCC5/FV/OVMF_CODE.fd,readonly=on \
  -drive if=pflash,format=raw,file=Build/OvmfX64/DEBUG_GCC5/FV/OVMF_VARS.fd \
  -drive format=raw,file=fat:rw:"${HDA_CONTENTS}"
