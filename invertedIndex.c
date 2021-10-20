#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <err.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "invertedIndex.h"

// Part 1 Functions
char *normaliseWord(char *str);
InvertedIndexBST generateInvertedIndex(char *collectionFilename);
void printInvertedIndex(InvertedIndexBST tree);

// Part 2 Functions
TfIdfList calculateTfIdf(InvertedIndexBST tree, char *searchWord, int D);
TfIdfList retrieve(InvertedIndexBST tree, char *searchWords[], int D);

// Helper functions
InvertedIndexBST newInvertedIndexBST(void);
InvertedIndexBST newInvertedIndexNode(char *v, char *filename);
InvertedIndexBST invertedIndexBSTreeInsert(InvertedIndexBST tree, char *word, char *filename);
FileList newFileListNode(char *v, char *filename);
void selectionSortAscending(struct FileListNode *head);
void selectionSortDescending(struct TfIdfNode *head);
void swapFilename(struct TfIdfNode *p1, struct TfIdfNode *p2);
void swapFileList(struct FileListNode *p1, struct FileListNode *p2);
void swapTfIdfList(struct TfIdfNode *p1, struct TfIdfNode *p2);
void printTree(InvertedIndexBST t, FILE *stream);
bool filenameinTfIdfList(struct TfIdfNode *head, char *str);
bool filenameinFileList(struct FileListNode *head, char *str);
double listSize(struct FileListNode *head);
InvertedIndexBST findWord(InvertedIndexBST root, char *key);
double fileWords(char *filename);
double countFrequency(char *word, char *filename);

////////////////////////////////////////////////////////
//                  PART 1 FUNCTIONS                  //
////////////////////////////////////////////////////////

char *normaliseWord(char *str) {

    // Remove leading whitespace
    int index = 0;
    while(str[index] == ' ' || str[index] == '\t' || str[index] == '\n') {
        index++;
    }
    // Shift all trailing characters to the left 
    int i = 0;
    while(str[i + index] != '\0') {
        str[i] = str[i + index];
        i++;
    }
    // Terminate the string
    str[i] = '\0'; 

    // Remove trailing whitespace
    i = 0;
    index = -1;
    while(str[i] != '\0') {
        if(str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
            index = i;
        }
        i++;
    }
    // Mark the next character to last non white space character as NULL 
    str[index + 1] = '\0';

    // Converting all characters to lowercase
    i = 0;
    while (str[i] != '\0') {
        str[i] = tolower(str[i]);
        i++;
    }
    // Removing punctuation at the end of a string
    int length = strlen(str);
    if (str[length - 1] == '.' || str[length - 1] == ',' || str[length - 1] == ';' || str[length - 1] == '?') {
        str[length - 1] = '\0';
    }
    return str;
}

InvertedIndexBST generateInvertedIndex(char *collectionFilename) {

    InvertedIndexBST new = newInvertedIndexBST();
    FILE *fp = fopen(collectionFilename, "r");
    char singleFilename[100];
    while (fscanf(fp, "%s", singleFilename) != EOF) {
        FILE *fp2 = fopen(singleFilename, "r");
        char word[100];
        while(fscanf(fp2, "%s", word) != EOF) {
            char *normalised_word = normaliseWord(word);
            new = invertedIndexBSTreeInsert(new, normalised_word, singleFilename);
        }
    }
    return new; 
}

void printInvertedIndex(InvertedIndexBST tree) {

    FILE *fp = fopen("invertedIndex.txt", "w");
    assert(fp != NULL);
    printTree(tree, fp);
}

////////////////////////////////////////////////////////
//                  PART 2 FUNCTIONS                  //
////////////////////////////////////////////////////////

TfIdfList calculateTfIdf(InvertedIndexBST tree, char *searchWord, int D) {

    InvertedIndexBST node = findWord(tree, searchWord);
    struct TfIdfNode *head = NULL;
    struct FileListNode *curr = node->fileList;
    double num_docs_containing_term = listSize(node->fileList);
    double idf = log10(D / num_docs_containing_term);
    while (curr != NULL) {
        struct TfIdfNode *n = malloc(sizeof *n);
        assert(n != NULL);
        double tf = curr->tf;
        double tf_idf = tf * idf;
        n->tfIdfSum = tf_idf;
        n->filename = malloc(100);
        strcpy(n->filename, curr->filename);
        n->next = NULL;
        // Insert TfIdfNode into TfIdfList
        if (head == NULL) {
            head = n;
        } else {
            struct TfIdfNode *temp = head;
            while (head->next != NULL) {
                head = head->next;
            }
            head->next = n;
            // Reset head back to the start of the list
            head = temp;
        }
        curr = curr->next;
    }
    // Sort the list in descending order of tf-idf value
    selectionSortDescending(head);
    return head;
}

TfIdfList retrieve(InvertedIndexBST tree, char *searchWords[], int D) {

    struct TfIdfNode *head = NULL;
    int i = 0;
    while (searchWords[i] != NULL) {
        InvertedIndexBST node = findWord(tree, searchWords[i]);
        struct FileListNode *curr = node->fileList;
        double num_docs_containing_term = listSize(node->fileList);
        double idf = log10(D / num_docs_containing_term);
        while (curr != NULL) {
            // Filename is not in the TfIdfList, insert the TfIdfNode in the list as usual
            if (filenameinTfIdfList(head, curr->filename) == false) {
                struct TfIdfNode *n = malloc(sizeof *n);
                n->filename = malloc(100);
                strcpy(n->filename, curr->filename);
                n->tfIdfSum = curr->tf * idf;
                n->next = NULL;
                if (head == NULL) {
                    head = n;
                }
                else {
                    struct TfIdfNode *temp = head;
                    while (head->next != NULL) {
                        head = head->next;
                    }
                    head->next = n;
                    // Reset head back to the start of the list
                    head = temp;
                }
            } else { // Filename already exists in the TfIdfList, add the current tf-idf value onto the existing tfIdfSum
                struct TfIdfNode *temp = head;
                while (temp != NULL) {
                    if (strcmp(curr->filename, temp->filename) == 0) {
                        double curr_tf_idf = curr->tf * idf;
                        temp->tfIdfSum += curr_tf_idf;
                    }
                    temp = temp->next;
                }
            }
            curr = curr->next;
        }
        i++;
    }
    // Sort the list in descending order of tfIdfSum
    selectionSortDescending(head);
    return head;
}

////////////////////////////////////////////////////////
//                  HELPER FUNCTIONS                  //
////////////////////////////////////////////////////////

// Returns true if a filename is in the TfIdfList and false if it isn't
bool filenameinTfIdfList(struct TfIdfNode *head, char *str) {

    while (head != NULL) {
        if (strcmp(head->filename, str) == 0) {
            return true;
        } 
        head = head->next;
    }
    return false;
}

// Returns true if a filename is in the FileList and false if it isn't
bool filenameinFileList(struct FileListNode *head, char *str) {

    while (head != NULL) {
        if (strcmp(head->filename, str) == 0) {
            return true;
        } 
        head = head->next;
    }
    return false;
}

// Sorts a FileList in ascending order using selection sort
void selectionSortAscending(struct FileListNode *head) {

    struct FileListNode *start = head;
	struct FileListNode *traverse;
	struct FileListNode *min;
	
	while (start->next) {
		min = start;
		traverse = start->next;
		while (traverse) {
			// Find minimum element from array  
			if (strcmp(min->filename, traverse->filename) > 0) {
				min = traverse;
			}
			traverse = traverse->next;
		}
        // Put min element on starting location
		swapFileList(start, min);			
		start = start->next;
    }
}

// Sorts the TfIdfList in descending order using selection sort
void selectionSortDescending(struct TfIdfNode *head) {

    struct TfIdfNode *start = head;
	struct TfIdfNode *traverse;
	struct TfIdfNode *max;
	
	while (start->next != NULL) {
		max = start;
		traverse = start->next;
		while (traverse != NULL) {
			// Find maximum element from array  
			if (max->tfIdfSum < traverse->tfIdfSum) {
				max = traverse;
			}
            // Equal tfIdfSum, arrange filename alphabetically
            if (max->tfIdfSum == traverse->tfIdfSum) {
                if (strcmp(max->filename, traverse->filename) > 0) {
                    swapFilename(max, traverse);
                }
            }
			traverse = traverse->next;
		}
        // Put max element on starting location
		swapTfIdfList(start, max);			
		start = start->next;
    }
}

// Swaps the filename of two TfIdfNodes
void swapFilename(struct TfIdfNode *p1, struct TfIdfNode *p2) {

	char *temp = p1->filename;
	p1->filename = p2->filename;
	p2->filename = temp;
}

// Swaps data of Filelist
void swapFileList(struct FileListNode *p1, struct FileListNode *p2) {

	char *temp_filename = p1->filename;
	p1->filename = p2->filename;
	p2->filename = temp_filename;
    double temp_tf = p1->tf;
    p1->tf = p2->tf;
    p2->tf = temp_tf;
}

// Swaps data of TfIdfList
void swapTfIdfList(struct TfIdfNode *p1, struct TfIdfNode *p2) {

	char *temp_filename = p1->filename;
	p1->filename = p2->filename;
	p2->filename = temp_filename;
    double temp_tfidf_sum = p1->tfIdfSum;
    p1->tfIdfSum = p2->tfIdfSum;
    p2->tfIdfSum = temp_tfidf_sum;
}

// Counts how many elements are in a FileList
double listSize(struct FileListNode *head) {

    double count = 0;
    while (head != NULL) {
        count++;
        head = head->next;
    }
    return count;
}

// Given a word, find the corresponding InvertedIndexBST node of that word
// Adapted from lab03 BSTree.c
InvertedIndexBST findWord(InvertedIndexBST root, char *key) {

    if (root == NULL || strcmp(root->word, key) == 0) {
        return root;
    }
    if (strcmp(key, root->word) > 0) {
        return findWord(root->right, key);
    }
    return findWord(root->left, key);
}

// Prints a BST to a specified stream
void printTree(InvertedIndexBST t, FILE *stream) {

    if (t == NULL) {
        return;
    }
    printTree(t->left, stream);
    fprintf(stream, "%s ", t->word);
    // Sort fileList in alphabetical order and print them
    selectionSortAscending(t->fileList);
    struct FileListNode *curr = t->fileList;
    while (curr != NULL) {
        fprintf(stream, "%s ", curr->filename);
        curr = curr->next;
    }
    fprintf(stream, "\n");
    printTree(t->right, stream);
}

// Creates a new InvertedIndexBST (adapted from lab03 BSTree.c)
InvertedIndexBST newInvertedIndexBST(void) {
    return NULL;
}

// Insert a new value into an invertedIndexBST (adapted from lab03 BSTree.c)
InvertedIndexBST invertedIndexBSTreeInsert(InvertedIndexBST tree, char *word, char *filename) {

    // Empty invertedIndexBST   
	if (tree == NULL) {
        InvertedIndexBST tree_node = newInvertedIndexNode(word, filename);
        tree_node->fileList = newFileListNode(word, filename);
        return tree_node;
    }
	else if (strcmp(word, tree->word) < 0) {
		tree->left = invertedIndexBSTreeInsert(tree->left, word, filename);
    }
	else if (strcmp(word, tree->word) > 0) {
		tree->right = invertedIndexBSTreeInsert(tree->right, word, filename);
    } else { // Duplicate word - only insert the filename, if it is not already in the fileList
        if (filenameinFileList(tree->fileList, filename) == false) {
            FileList curr = tree->fileList;
            while (curr->next != NULL) {
                curr = curr->next;
            }
            struct FileListNode *new = newFileListNode(word, filename);
            curr->next = new; 
        }
    } 
    return tree;
}

// Make a new InvertedIndexNode containing a value v (adapted from lab01 IntList.c)
InvertedIndexBST newInvertedIndexNode(char *v, char *filename) {

	struct InvertedIndexNode *new = malloc(sizeof(struct InvertedIndexNode));
    assert(new != NULL);
	new->word = malloc(100);
    strcpy(new->word, v);
	new->left = NULL;
    new->right = NULL;
	return new;
} 

// Make a new FileListNode containing a value v (adapted from lab01 IntList.c)
FileList newFileListNode(char *v, char *filename) {

	struct FileListNode *n = malloc(sizeof(struct FileListNode));
    assert(n != NULL);
	n->filename = malloc(100);
    strcpy(n->filename, filename);
    n->tf = countFrequency(v, filename) / fileWords(filename);
	n->next = NULL;
	return n;
}

// Counts the number of words in a file
double fileWords(char *filename) {

    double count = 0;
    FILE *fp = fopen(filename, "r");
    char buf[100];
    while(fscanf(fp, "%s", buf) != EOF) {
        count++;
    }
    fclose(fp);
    return count;
}

// Counts the number of times a certain word appears in a file
double countFrequency(char *word, char *filename) {

    double count = 0;
    FILE *fp = fopen(filename, "r");
    char buf[100];
    while(fscanf(fp, "%s", buf) != EOF) {
        char *normalised_word = normaliseWord(buf);
        if (strcmp(normalised_word, word) == 0) {
            count++;
        }
    }
    fclose(fp);
    return count;
}