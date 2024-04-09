This script is crude attempt to fix way in which IDA create unitialized data
sybols from ELF .symtab segment.



IDA, by default, in common segement, for each symbol, create DWORD value
containig symbol size. Each xrefs for this symbol is directed to this value.
Using fixups it is corrected to external variable. This cause that any offset
added to this symbol cannot by treated as struct or array element. This make
code analysis much harder.



This scrpit create new data segment big enoug to fit all data described in
common symbol. Then for each symbol in common segment do:

- create new data in new segment of size described in old one;

- rename old symbold to NAME_ref and give new symbol name NAME;

- patch code in text segment to point to new symbol;

- delete fixups in text segment for this symbol;

- move data xrefs for old one to new one;

This allow to define offsets of this new sybols as struct or array element,
making code analysis easier.



How to use:

- Make backup of your database. Running this script may heavly damage it.

- Run File->Script File... and choose this script.

- When scipts asks, put any addres within IDA common segment to identify it.

- Now scripts run. It may take several seconds.

- Optionaly reanalyze database. Script will ask, if you want to do it. It's
recommended to do it, as new data xrefs can be created from now. If you decline,
you can do it later by choosing: Options->General->Analysis->Reanalyze program.

- When done, check if new segment is created properly and (hopefully) all
data xref are moved to it.

- In case of unsedesirable effects, close database WITHOUT SAVING!



Limitations:

- This code works probably only for 32bit binary.

- Some xrefs may not be moved due to conservative move conditions.

- There are no safety checks. I.e. choosing wrong segment may broke database
entirely.