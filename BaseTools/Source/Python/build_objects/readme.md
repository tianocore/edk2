# Build Objects

## What are they?

Data model or build objects are data objects that allow for a DSC or other build file to be converted into an python object.
This allows transformations, verifications, and other things to do be done much more easily as they can be done in-memory.
The current proposal is three separate data models, detailed below.
Generally the data model is composed of sets and maps of sets.
The sets will be checked for uniqueness and will be restricted in what can be added to them.
The maps will have various section header types as keys and restricted sets as values.

### DSC

This is the data model that represents the DSC file, macros fully resolved with conditional paths taken.
This is a fairly standard 1:1 mapping from the spec to the DSC object.
No higher level verification is to be done by the data model object itself (for example, checking that the SKU that a PCD references is in fact legal or INF paths exist).
This higher level verification can be done by a separate class, perhaps once the object has been created in the parser itself.
Low level verifications (for example, anything specified with specific values in the spec) may be done as the file is being processed as long as it can be immediately verified.
A good example of this would be checking that the module type specified exists in the list of allowed EDKII values (DXE_RUNTIME_DRIVER, PEIM, etc).

### FDF

This is the data model that represents the FDF file, macros fully resolved with conditional paths taken.
Similar to DSCs, no higher level verification should be done in the data model itself.
Low lever verifications may be done by the data model.

### Recipe

The central class is the `recipe`.
This holds all the components that need to be built and their respective library classes, PCD's, and defines.
It does not contain general library classes.
In the future, it will also contain the flash map information.


We've written a parser for DSC files to convert them into recipes.
In the future, we hope to include FDF files as well.

## Who are they for?

Build objects are for anyone dealing with complex and large projects.
In projects, more and more DSC files are taking advantage of the !include functionality. However, there are a few problems with that fact.
1. DSC files are fragile
2. Includes have no idea what is already in your file. An include might expect you to be in a defines section. You have no way to know this from the main DSC file.
3.

## Why were they made?

To better abstract away the essence of what a build is doing. DSC is a way to communicate a recipe.

## How are they being used?

- DSC compositing/transformations
- Build state verification
- Best practice checking