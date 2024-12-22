#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <direct.h>
  #define CHDIR _chdir
#else
  #include <unistd.h>
  #define CHDIR chdir
#endif

#define MAX_LINES       25
#define MAX_LINE_LENGTH 1024
#define STACK_SIZE      3

typedef enum {
    OP_NONE,
    OP_INSERT_LINE,
    OP_DELETE_LINE,
    OP_UPDATE_LINE,
    OP_INSERT_WORD,
    OP_DELETE_WORD
} OperationType;

typedef struct {
    OperationType opType;
    int   lineIndex;
    char *oldLine;
    char *newLine;
    int   charPos;
    char *oldWord;
    char *newWord;
    int   wordLength;
} OperationRecord;

char *g_lines[MAX_LINES];
int   g_lineCount   = 0;
char  g_filePath[1024];

OperationRecord g_undoStack[STACK_SIZE];
OperationRecord g_redoStack[STACK_SIZE];
int g_undoTop = -1;
int g_redoTop = -1;

char* safeStrdup(const char *src) {
    if (!src) return NULL;
    char *copy = (char *)malloc(strlen(src) + 1);
    if (!copy) return NULL;
    strcpy(copy, src);
    return copy;
}

void freeOperationRecord(OperationRecord *op) {
    if (!op) return;
    free(op->oldLine);  op->oldLine  = NULL;
    free(op->newLine);  op->newLine  = NULL;
    free(op->oldWord);  op->oldWord  = NULL;
    free(op->newWord);  op->newWord  = NULL;
}

void usageAndExit() {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  editor.exe [filename] [directory]\n");
    exit(EXIT_FAILURE);
}

char* readLineFromFile(FILE *fp) {
    char buffer[MAX_LINE_LENGTH];
    if (!fp) return NULL;
    if (fgets(buffer, sizeof(buffer), fp)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        char *line = (char *)malloc(len + 1);
        if (line) {
            strcpy(line, buffer);
            return line;
        }
    }
    return NULL;
}

void loadFile(const char *filePath) {
    FILE *fp = fopen(filePath, "r");
    if (!fp) {
        g_lineCount = 0;
        return;
    }
    g_lineCount = 0;
    while (g_lineCount < MAX_LINES) {
        char *line = readLineFromFile(fp);
        if (!line) break;
        g_lines[g_lineCount++] = line;
    }
    fclose(fp);
}

void saveFile(const char *filePath) {
    FILE *fp = fopen(filePath, "w");
    if (!fp) {
        printf("Error: Unable to open file for writing: %s\n", filePath);
        return;
    }
    for (int i = 0; i < g_lineCount; i++) {
        if (g_lines[i]) {
            fprintf(fp, "%s\n", g_lines[i]);
        }
    }
    fclose(fp);
    printf("File saved: %s\n", filePath);
}

void pushUndo(OperationRecord op) {
    if (g_undoTop == STACK_SIZE - 1) {
        freeOperationRecord(&g_undoStack[0]);
        for (int i = 0; i < STACK_SIZE - 1; i++) {
            g_undoStack[i] = g_undoStack[i+1];
        }
        g_undoTop--;
    }
    g_undoTop++;
    g_undoStack[g_undoTop] = op;
    for (int i = 0; i <= g_redoTop; i++) {
        freeOperationRecord(&g_redoStack[i]);
        g_redoStack[i].opType = OP_NONE;
    }
    g_redoTop = -1;
}

void pushRedo(OperationRecord op) {
    if (g_redoTop == STACK_SIZE - 1) {
        freeOperationRecord(&g_redoStack[0]);
        for (int i = 0; i < STACK_SIZE - 1; i++) {
            g_redoStack[i] = g_redoStack[i+1];
        }
        g_redoTop--;
    }
    g_redoTop++;
    g_redoStack[g_redoTop] = op;
}

OperationRecord popUndo() {
    if (g_undoTop < 0) {
        OperationRecord nullOp;
        memset(&nullOp, 0, sizeof(nullOp));
        nullOp.opType = OP_NONE;
        return nullOp;
    }
    OperationRecord op = g_undoStack[g_undoTop];
    g_undoTop--;
    return op;
}

OperationRecord popRedo() {
    if (g_redoTop < 0) {
        OperationRecord nullOp;
        memset(&nullOp, 0, sizeof(nullOp));
        nullOp.opType = OP_NONE;
        return nullOp;
    }
    OperationRecord op = g_redoStack[g_redoTop];
    g_redoTop--;
    return op;
}

int searchWord(const char *word) {
    if (!word || strlen(word) == 0) return -1;
    for (int i = 0; i < g_lineCount; i++) {
        char *found = strstr(g_lines[i], word);
        if (found) {
            int pos = (int)(found - g_lines[i]);
            printf("Word '%s' found at line %d, position %d\n", word, i+1, pos);
            return 0;
        }
    }
    printf("Word '%s' not found.\n", word);
    return -1;
}

void insertLine(int index, const char *text) {
    if (index < 0 || index > MAX_LINES) {
        printf("Invalid line index!\n");
        return;
    }
    while (g_lineCount < index && g_lineCount < MAX_LINES) {
        g_lines[g_lineCount] = safeStrdup("");
        g_lineCount++;
    }
    if (g_lineCount == MAX_LINES && index == MAX_LINES) {
        printf("Buffer full!\n");
        return;
    }
    OperationRecord op;
    memset(&op, 0, sizeof(op));
    op.opType    = OP_INSERT_LINE;
    op.lineIndex = index;
    op.newLine   = safeStrdup(text ? text : "");
    for (int i = g_lineCount; i > index; i--) {
        g_lines[i] = g_lines[i-1];
    }
    g_lines[index] = safeStrdup(text ? text : "");
    g_lineCount++;
    pushUndo(op);
    printf("Inserted new line at %d.\n", index+1);
}

void readLineFromBuffer(int index) {
    if (index < 0 || index >= g_lineCount) {
        printf("Invalid line number.\n");
        return;
    }
    printf("Line %d: %s\n", index+1, g_lines[index]);
}

void readAllLinesFromBuffer() {
    printf("------ Buffer Start ------\n");
    for (int i = 0; i < g_lineCount; i++) {
        printf("Line %d: %s\n", i+1, g_lines[i]);
    }
    printf("------ Buffer End   ------\n");
}

void updateLine(int index, const char *newContent) {
    if (index < 0 || index >= g_lineCount) {
        printf("Invalid line number.\n");
        return;
    }
    OperationRecord op;
    memset(&op, 0, sizeof(op));
    op.opType    = OP_UPDATE_LINE;
    op.lineIndex = index;
    op.oldLine   = safeStrdup(g_lines[index]);
    op.newLine   = safeStrdup(newContent ? newContent : "");
    free(g_lines[index]);
    g_lines[index] = safeStrdup(op.newLine);
    pushUndo(op);
    printf("Updated line %d.\n", index+1);
}

void deleteLine(int index) {
    if (index < 0 || index >= g_lineCount) {
        printf("Invalid line number.\n");
        return;
    }
    OperationRecord op;
    memset(&op, 0, sizeof(op));
    op.opType    = OP_DELETE_LINE;
    op.lineIndex = index;
    op.oldLine   = safeStrdup(g_lines[index]);
    free(g_lines[index]);
    for (int i = index; i < g_lineCount-1; i++) {
        g_lines[i] = g_lines[i+1];
    }
    g_lines[g_lineCount-1] = NULL;
    g_lineCount--;
    pushUndo(op);
    printf("Deleted line %d.\n", index+1);
}

void insertWordAtPosition(int lineIdx, int charPos, const char *word) {
    if (lineIdx < 0 || lineIdx >= g_lineCount) {
        printf("Invalid line number.\n");
        return;
    }
    if (!word) {
        printf("Nothing to insert.\n");
        return;
    }
    int lineLen = (int)strlen(g_lines[lineIdx]);
    if (charPos < 0 || charPos > lineLen) {
        printf("Character position out of range (0..%d).\n", lineLen);
        return;
    }
    OperationRecord op;
    memset(&op, 0, sizeof(op));
    op.opType     = OP_INSERT_WORD;
    op.lineIndex  = lineIdx;
    op.charPos    = charPos;
    op.oldLine    = safeStrdup(g_lines[lineIdx]);
    op.newWord    = safeStrdup(word);
    op.wordLength = (int)strlen(word);
    char *original = g_lines[lineIdx];
    int newSize = lineLen + op.wordLength + 1;
    char *newLine = (char *)malloc(newSize);
    if (!newLine) {
        printf("Memory fail.\n");
        return;
    }
    strncpy(newLine, original, charPos);
    newLine[charPos] = '\0';
    strcat(newLine, word);
    strcat(newLine, &original[charPos]);
    free(g_lines[lineIdx]);
    g_lines[lineIdx] = newLine;
    pushUndo(op);
    printf("Inserted word at line %d, position %d.\n", lineIdx+1, charPos);
}

void deleteWordAtPosition(int lineIdx, int startCharPos, int lengthToDelete) {
    if (lineIdx < 0 || lineIdx >= g_lineCount) {
        printf("Invalid line number.\n");
        return;
    }
    int lineLen = (int)strlen(g_lines[lineIdx]);
    if (startCharPos < 0 || startCharPos >= lineLen) {
        printf("Character position out of range.\n");
        return;
    }
    if (lengthToDelete <= 0) {
        printf("Invalid length.\n");
        return;
    }
    if (startCharPos + lengthToDelete > lineLen) {
        lengthToDelete = lineLen - startCharPos;
    }
    OperationRecord op;
    memset(&op, 0, sizeof(op));
    op.opType     = OP_DELETE_WORD;
    op.lineIndex  = lineIdx;
    op.charPos    = startCharPos;
    op.oldLine    = safeStrdup(g_lines[lineIdx]);
    op.wordLength = lengthToDelete;
    char *original = g_lines[lineIdx];
    char *newLine  = (char *)malloc(lineLen + 1);
    if (!newLine) {
        printf("Memory fail.\n");
        return;
    }
    strncpy(newLine, original, startCharPos);
    newLine[startCharPos] = '\0';
    strcat(newLine, &original[startCharPos + lengthToDelete]);
    free(g_lines[lineIdx]);
    g_lines[lineIdx] = newLine;
    pushUndo(op);
    printf("Deleted %d chars in line %d.\n", lengthToDelete, lineIdx+1);
}

static void applyInverse(const OperationRecord *op) {
    switch (op->opType) {
        case OP_INSERT_LINE:
        {
            int idx = op->lineIndex;
            free(g_lines[idx]);
            for (int i = idx; i < g_lineCount - 1; i++) {
                g_lines[i] = g_lines[i + 1];
            }
            g_lines[g_lineCount - 1] = NULL;
            g_lineCount--;
            break;
        }
        case OP_DELETE_LINE:
        {
            int idx = op->lineIndex;
            for (int i = g_lineCount; i > idx; i--) {
                g_lines[i] = g_lines[i - 1];
            }
            g_lines[idx] = safeStrdup(op->oldLine);
            g_lineCount++;
            break;
        }
        case OP_UPDATE_LINE:
        {
            int idx = op->lineIndex;
            free(g_lines[idx]);
            g_lines[idx] = safeStrdup(op->oldLine);
            break;
        }
        case OP_INSERT_WORD:
        {
            int idx = op->lineIndex;
            free(g_lines[idx]);
            g_lines[idx] = safeStrdup(op->oldLine);
            break;
        }
        case OP_DELETE_WORD:
        {
            int idx = op->lineIndex;
            free(g_lines[idx]);
            g_lines[idx] = safeStrdup(op->oldLine);
            break;
        }
        default:
            break;
    }
}

void doUndo() {
    OperationRecord op = popUndo();
    if (op.opType == OP_NONE) {
        printf("No operations to undo.\n");
        return;
    }
    OperationRecord redoOp;
    memset(&redoOp, 0, sizeof(redoOp));
    redoOp.opType     = op.opType;
    redoOp.lineIndex  = op.lineIndex;
    redoOp.charPos    = op.charPos;
    redoOp.wordLength = op.wordLength;
    redoOp.oldLine    = safeStrdup(op.oldLine);
    redoOp.newLine    = safeStrdup(op.newLine);
    redoOp.oldWord    = safeStrdup(op.oldWord);
    redoOp.newWord    = safeStrdup(op.newWord);
    pushRedo(redoOp);
    applyInverse(&op);
    freeOperationRecord(&op);
    printf("Undo done.\n");
}

static void applyOperation(const OperationRecord *op) {
    switch (op->opType) {
        case OP_INSERT_LINE:
            insertLine(op->lineIndex, op->newLine);
            break;
        case OP_DELETE_LINE:
            deleteLine(op->lineIndex);
            break;
        case OP_UPDATE_LINE:
            updateLine(op->lineIndex, op->newLine);
            break;
        case OP_INSERT_WORD:
        {
            free(g_lines[op->lineIndex]);
            g_lines[op->lineIndex] = safeStrdup(op->oldLine);
            insertWordAtPosition(op->lineIndex, op->charPos, op->newWord);
            break;
        }
        case OP_DELETE_WORD:
        {
            free(g_lines[op->lineIndex]);
            g_lines[op->lineIndex] = safeStrdup(op->oldLine);
            deleteWordAtPosition(op->lineIndex, op->charPos, op->wordLength);
            break;
        }
        default:
            break;
    }
}

void doRedo() {
    OperationRecord op = popRedo();
    if (op.opType == OP_NONE) {
        printf("No operations to redo.\n");
        return;
    }
    applyOperation(&op);
    pushUndo(op);
    printf("Redo done.\n");
}

void displayMenu() {
    printf("\n===== LINE EDITOR MENU =====\n");
    printf("1.  Read All Lines\n");
    printf("2.  Read Specific Line\n");
    printf("3.  Insert New Line\n");
    printf("4.  Update Line\n");
    printf("5.  Delete Line\n");
    printf("6.  Search Word\n");
    printf("7.  Insert Word at Position\n");
    printf("8.  Delete Word at Position\n");
    printf("9.  Undo\n");
    printf("10. Redo\n");
    printf("11. Save File\n");
    printf("12. Exit\n");
    printf("===========================\n");
    printf("Enter your choice: ");
}

void handleUserChoice(int choice) {
    int index;
    char buffer[MAX_LINE_LENGTH];
    switch (choice) {
        case 1:
            readAllLinesFromBuffer();
            break;
        case 2:
            printf("Enter line number: ");
            scanf("%d", &index);
            getchar();
            index--;
            readLineFromBuffer(index);
            break;
        case 3:
        {
            printf("Enter line number (1-based): ");
            scanf("%d", &index);
            getchar();
            index--;
            printf("Enter new line text: ");
            fgets(buffer, sizeof(buffer), stdin);
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n') {
                buffer[len-1] = '\0';
            }
            insertLine(index, buffer);
            break;
        }
        case 4:
        {
            printf("Enter line number: ");
            scanf("%d", &index);
            getchar();
            index--;
            printf("Enter new line text: ");
            fgets(buffer, sizeof(buffer), stdin);
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n') {
                buffer[len-1] = '\0';
            }
            updateLine(index, buffer);
            break;
        }
        case 5:
        {
            printf("Enter line number: ");
            scanf("%d", &index);
            getchar();
            index--;
            deleteLine(index);
            break;
        }
        case 6:
        {
            printf("Enter word to search: ");
            scanf("%s", buffer);
            getchar();
            searchWord(buffer);
            break;
        }
        case 7:
        {
            int lineNumber, charPos;
            printf("Enter line number (1-based): ");
            scanf("%d", &lineNumber);
            getchar();
            lineNumber--;
            printf("Enter character position (0-based): ");
            scanf("%d", &charPos);
            getchar();
            printf("Enter text to insert: ");
            fgets(buffer, sizeof(buffer), stdin);
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n') {
                buffer[len-1] = '\0';
            }
            insertWordAtPosition(lineNumber, charPos, buffer);
            break;
        }
        case 8:
        {
            int lineNumber, startPos, length;
            printf("Enter line number (1-based): ");
            scanf("%d", &lineNumber);
            lineNumber--;
            printf("Enter start char position: ");
            scanf("%d", &startPos);
            printf("Enter length to delete: ");
            scanf("%d", &length);
            getchar();
            deleteWordAtPosition(lineNumber, startPos, length);
            break;
        }
        case 9:
            doUndo();
            break;
        case 10:
            doRedo();
            break;
        case 11:
            saveFile(g_filePath);
            break;
        case 12:
            printf("Exiting...\n");
            exit(0);
        default:
            printf("Invalid choice.\n");
            break;
    }
}

int main(int argc, char *argv[]) {
    char filename[256] = "file.txt";
    char directory[512] = {0};
    if (argc == 1) {
        getcwd(g_filePath, sizeof(g_filePath));
        strcat(g_filePath, "/");
        strcat(g_filePath, filename);
    }
    else if (argc == 2) {
        strcpy(filename, argv[1]);
        getcwd(g_filePath, sizeof(g_filePath));
        strcat(g_filePath, "/");
        strcat(g_filePath, filename);
    }
    else if (argc == 3) {
        strcpy(filename, argv[1]);
        strcpy(directory, argv[2]);
        if (CHDIR(directory) != 0) {
            printf("Warning: Failed to chdir to %s.\n", directory);
            getcwd(g_filePath, sizeof(g_filePath));
        } else {
            getcwd(g_filePath, sizeof(g_filePath));
        }
        strcat(g_filePath, "/");
        strcat(g_filePath, filename);
    }
    else {
        usageAndExit();
    }
    printf("File path: %s\n", g_filePath);
    loadFile(g_filePath);
    while (1) {
        displayMenu();
        int choice;
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input, exiting.\n");
            break;
        }
        getchar();
        handleUserChoice(choice);
    }
    return 0;
}
