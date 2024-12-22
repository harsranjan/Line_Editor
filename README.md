# Line Editor (C Language)

A command-line line editor written in **C** that provides:
- File creation/opening (with command-line arguments for filename/directory)  
- An in-memory buffer for up to 25 lines of text  
- Search functionality for strings in the buffer  
- CRUD operations (insert line, delete line, update line)  
- Word-level insertion/deletion at any position in a given line (supports multiple words/spaces)  
- A 3-level Undo/Redo system  

## Features

1. **Command-Line Arguments**  
   - **No arguments**: Opens/creates `file.txt` in the current directory.  
   - **1 argument**: Opens/creates the specified filename in the current directory.  
   - **2 arguments**: First is the filename, second is a directory name. Attempts to open/create in that directory.

2. **Buffer**  
   - Up to **25 lines** stored in an array.  
   - Each line can be up to **1024 characters** long.

3. **CRUD Operations**  
   - **Insert Line**: Inserts a new line at the user-specified position (filling intermediate lines with empty strings if needed).  
   - **Update Line**: Replaces the entire content of a specific line.  
   - **Delete Line**: Removes a specific line from the buffer, shifting subsequent lines up.  
   - **Insert Word**: Inserts a given string at a specified character position in a line.  
   - **Delete Word**: Deletes a specified number of characters from a particular position in a line.

4. **Search**  
   - Searches the entire buffer for a word and returns the first line and character offset (0-based) where it appears.

5. **Undo/Redo**  
   - Maintains separate **Undo** and **Redo** stacks (up to 3 operations each).  
   - Each new operation is pushed to **Undo**, clearing **Redo**.  
   - **Undo** pops from **Undo** stack, applies the inverse operation, and pushes the original operation to **Redo**.  
   - **Redo** pops from **Redo** stack, reapplies the original operation, and pushes it to **Undo**.

## Usage

### Compilation

1. Open a terminal/command prompt.
2. Run:
   ```bash
   gcc line_editor.c -o editor
   ```
   Replace `line_editor.c` with your actual filename if necessary (e.g., `editor.c`).

### Running

- **No arguments** (uses `file.txt` in current directory):
  ```bash
  ./editor
  ```
- **One argument** (opens/creates `myfile.txt` in current directory):
  ```bash
  ./editor myfile.txt
  ```
- **Two arguments** (filename + directory):
  ```bash
  ./editor myfile.txt folderPath
  ```

After launching, you'll see a **menu** in the console:

```
===== LINE EDITOR MENU =====
1.  Read All Lines
2.  Read Specific Line
3.  Insert New Line
4.  Update Line
5.  Delete Line
6.  Search Word
7.  Insert Word at Position
8.  Delete Word at Position
9.  Undo
10. Redo
11. Save File
12. Exit
===========================
Enter your choice:
```

Enter the **number** corresponding to the action you want to perform. You will be prompted for additional information (like line number, text to insert, etc.). 

### Example Flow

1. **Insert a New Line** (menu option 3)
   - Provide the line number (1-based) and the content.  
   - This is pushed onto the Undo stack.
2. **Insert Another Line**  
3. **Search** for a word (option 6).  
4. **Undo** one of those operations (option 9).  
   - The undone operation is moved to the Redo stack.  
5. **Redo** (option 10) to re-apply the undone operation.  

When you're done making changes:

- Choose **11** to **Save File**.  
- Choose **12** to **Exit**.

### Undo/Redo Notes

- **Undo** operates on the **most recent** operation you performed.  
- **Redo** re-applies the last undone operation (if any) from the Redo stack.  
- Each stack can hold **up to 3** operations. Attempting to push more discards the oldest entry in that stack.  
- Any **new** operation clears the Redo stack (standard editor behavior).

### File Behavior

- **Loading**: If the file doesn’t exist, the editor starts with an **empty** buffer.  
- **Saving**: Overwrites the file with the in-memory buffer content.  

## Contributing / Extending

- Increase `MAX_LINES` or `MAX_LINE_LENGTH` if needed.  
- Expand Undo/Redo capacity by adjusting `STACK_SIZE`.  
- Add more advanced editing commands or additional validations as needed.

## License

This project is provided as-is, with no warranty. You can adapt or enhance it for your own purposes.
