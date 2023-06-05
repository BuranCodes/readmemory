# readmemory

## Overview

The `readmemory` program is designed to extract strings from the stack and heap memory of a specified process. It provides a way to analyze the memory of a running process and retrieve string data for further analysis or debugging purposes.

## Usage

```css
readmemory [procID]
```

* `procID`: The process ID of the target process. If not provided, the program will display a prompt, asking for process ID.

Note: The `readmemory` program generates text files containing the extracted strings. The files are named "ptrstringDump.txt" for pointer strings, "localstringDump.txt" for local strings, and "heapstringDump.txt" for heap strings.

## Functionality

The `readmemory` program performs the following steps:

1. Opens the target process using the specified process ID.
2. Retrieves information about the target process and its threads.
3. Searches for strings in the heap memory of the target process and writes them to "heapstringDump.txt".
4. Searches for strings in the stack memory of each thread in the target process and writes them to "localstringDump.txt" and "ptrstringDump.txt".
5. Closes the target process.
6. Displays a success message indicating that the text files have been generated.

## Output Files

The `readmemory` program generates the following output files:

* `heapstringDump.txt`: Contains the extracted strings from the heap memory of the target process.
* `localstringDump.txt`: Contains the extracted strings from the stack memory of the target process (local variables).
* `ptrstringDump.txt`: Contains the extracted strings from the stack memory of the target process (pointers).

## Dependencies

* `winternl.h`: Provides the necessary definitions for Windows internal functions.
* `windows.h`: Includes various Windows API declarations and definitions.
* `stdio.h`: Provides input/output functions for file operations.
* `string.h`: Includes functions for string manipulation and memory operations.
* `inttypes.h`: Defines format specifiers for integer types.

## Notes

* The program uses the `NtOpenThread` function from the "ntdll.dll" library to obtain thread information.
* The program relies on Windows API functions, such as `ReadProcessMemory` and `VirtualQueryEx`, to read and analyze the memory of the target process.
* The program requires sufficient privileges to access and read the memory of the target process.

## Examples

To analyze the memory of a process with ID 1234, run the following command:

```c
readmemory 1234
```

## Additional Information

For more details about the implementation and inner workings of the `readmemory` program, refer to the source code comments and consult the documentation of the [Windows API functions](https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-api-list) used.

## **Contributing**

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement". Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch `(git checkout -b feature/epicfeature)`
3. Commit your Changes `(git commit -m 'Add some epic feature')`
4. Push to the Branch `(git push origin feature/epicfeature)`
5. Open a Pull Request
