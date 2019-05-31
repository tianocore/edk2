# SplitFspBin.py is a python script to support some operations on Intel FSP 2.0 image.

It supports:

- Split Intel FSP 2.0 image into individual FSP-T/M/S/O component

- Rebase Intel FSP 2.0 components to different base addresses

- Generate Intel FSP 2.0 C header file

- Display Intel FSP 2.0 information header for each FSP component

## Split Intel FSP 2.0 image

To split individual FSP component in Intel FSP 2.0 image, the following
command can be used:

   **python SplitFspBin.py split [-h] -f FSPBINARY [-o OUTPUTDIR] [-n NAMETEMPLATE]**

For example:  

   `python SplitFspBin.py split -f FSP.bin`

   It will create FSP_T.bin, FSP_M.bin and FSP_S.bin in current directory.

## Rebase Intel FSP 2.0 components

To rebase one or multiple FSP components in Intel FSP 2.0 image, the following
command can be used:

   **python SplitFspBin.py rebase [-h] -f FSPBINARY -c {t,m,s,o} [{t,m,s,o} ...] -b FSPBASE [FSPBASE ...] [-o OUTPUTDIR] [-n OUTPUTFILE]**

For example:  

   `python SplitFspBin.py rebase -f FSP.bin –c t –b 0xFFF00000 –n FSP_new.bin`

   It will rebase FSP-T component inside FSP.bin to new base 0xFFF00000 and save the
   rebased Intel FSP 2.0 image into file FSP_new.bin.

   `python SplitFspBin.py rebase -f FSP.bin –c t m –b 0xFFF00000 0xFEF80000 –n FSP_new.bin`

   It will rebase FSP-T and FSP-M components inside FSP.bin to new base 0xFFF00000
   and 0xFEF80000 respectively, and save the rebased Intel FSP 2.0 image into file 
   FSP_new.bin file.

## Generate Intel FSP 2.0 C header file

To generate Intel FSP 2.0 C header file, the following command can be used:

   **Python SplitFspBin.py genhdr [-h] -f FSPBINARY [-o OUTPUTDIR] [-n HFILENAME]**

For example:  

   `python SplitFspBin.py genhdr -f FSP.bin –n FSP.h`

   It will create the C header file FSP.h containing the image ID, revision, offset
   and size for each individual FSP component.

## Display Intel FSP 2.0 information header

To display Intel FSP 2.0 information headers, the following command can be used:

   **Python SplitFspBin.py info [-h] -f FSPBINARY**

For example:  

   `python SplitFspBin.py info -f FSP.bin`

   It will print out the FSP information header for each FSP component.
