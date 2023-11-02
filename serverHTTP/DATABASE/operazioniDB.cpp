#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX_NAME_LENGTH 30
#define MAX_EMAIL_LENGTH 320
#define MAX_PASSWORD_LENGTH 128 

#define ARCHIVE_FILE "../DATABASE/Archivio.dat"
#define INDEX_FILE "../DATABASE/Indice.dat"

#pragma warning(disable:4996)

typedef struct {
    char username[MAX_NAME_LENGTH];
    char email[MAX_EMAIL_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
} user;

typedef struct {
    char email[MAX_EMAIL_LENGTH];
    int pos;
} indexedUser;

FILE* fileIndice;
FILE* fileArchivio;


// Functions to support indexed operations
int indexedSearchRecord(char email[], FILE* fileIndice);
int indexedInsertRecord(user nuovoRecord);
int indexedDeleteRecord(char email[], FILE* fileArchivio, FILE* fileIndice);


/*
int main(int argc, char* argv[]) {
    int choice = atoi(argv[1]);

    char email[MAX_EMAIL_LENGTH];
    user nuovoRecord;



    FILE *fileArchivio = fopen(ARCHIVE_FILE, "ab+");
    if (fileArchivio == NULL) {
        perror("Error opening fileArchivio");
        return 1;
    }

    FILE *fileIndice = fopen(INDEX_FILE, "ab+");
    if (fileIndice == NULL) {
        perror("Error opening fileIndice");
        return 1;
    }



    switch (choice) {
        case 2: {

            if(argc<3){
                return -1;
            }
            
            int position = indexedSearchRecord(argv[2], fileIndice);
            if (position != -1) {
                printf("Record found at position %d.\n", position);
            } else {
                printf("Record not found.\n");
            }

            break;
        }
        case 3: {
            printf("Enter the data for the new record:\n");
            printf("Key: ");
            scanf("%s", nuovoRecord.email);


            int result = indexedInsertRecord(nuovoRecord, fileArchivio, fileIndice);
            if (result) {
                printf("Record inserted successfully.\n");
            } else {
                printf("Unable to insert the record (duplicate key).\n");
            }

            break;
        }
        case 4: {
            printf("Enter the key of the record to delete: ");
            scanf("%s", email);
            int deleted = indexedDeleteRecord(email, fileIndice,fileArchivio);
            if (deleted) {
                printf("Record deleted successfully.\n");
            } else {
                printf("Record not found.\n");
            }
            break;
        }
        case 5: {
            fclose(fileArchivio);
            fclose(fileIndice);
            return 0;
        }
        default: {
            printf("Invalid choice. Try again.\n");
        }
    }
    fclose(fileArchivio);
    fclose(fileIndice);
    

    return 0;
}
*/

int indexedSearchRecordRecursive(char email[], FILE* fileIndice, int start, int end) {
    // Check if the search range is valid.
    if (start <= end) {
        indexedUser indexedRecord;
        // Calculate the middle index.
        int mid = (start + end) / 2;
        // Position the file pointer at the middle of the file.
        fseek(fileIndice, mid * sizeof(indexedUser), SEEK_SET);
        // Read one record from the file into indexedRecord.
        fread(&indexedRecord, sizeof(indexedUser), 1, fileIndice);

        // Compare the read key with the search key.
        int compare = strcmp(indexedRecord.email, email);

        if (compare == 0) {
            // Return the position if the record is found.
            return indexedRecord.pos; // Record trovato, restituisci la posizione
        }
        else if (compare < 0) {
            // Recursively search the right half of the range.
            return indexedSearchRecordRecursive(email, fileIndice, mid + 1, end);
        }
        else {
            // Recursively search the left half of the range.
            return indexedSearchRecordRecursive(email, fileIndice, start, mid - 1);
        }
    }


    return -1; // Record not found
}


int indexedSearchRecord(char email[], FILE* fileIndice) {
    // Calculate the size of the file
    fseek(fileIndice, 0, SEEK_END);
    int fileSize = ftell(fileIndice);
    int start = 0;
    int end = fileSize / sizeof(indexedUser) - 1;

    int result = indexedSearchRecordRecursive(email, fileIndice, start, end);


    return result; // record position or -1 if not found 
}


int loginAuthentication(char email[], char password[]) {
    fileArchivio = fopen(ARCHIVE_FILE, "ab+");
    if (fileArchivio == NULL) {
        perror("Error opening fileArchivio");
        return 1;
    }

    fileIndice = fopen(INDEX_FILE, "ab+");
    if (fileIndice == NULL) {
        perror("Error opening fileIndice");
        return 1;
    }

    int pos;
    user curUser;

    // if not found
    pos = indexedSearchRecord(email, fileIndice);
    if (pos == -1) {
        fclose(fileArchivio);
        fclose(fileIndice);
        return 1; //login non valido
    }


    fseek(fileArchivio, (pos * sizeof(user)),SEEK_SET);
    fread(&curUser, sizeof(user), 1, fileArchivio);
    if (strcmp(curUser.email,email)==0&&strcmp(curUser.password,password)==0) {
        fclose(fileArchivio);
        fclose(fileIndice);
        return 0; // login valido
    }else {
        fclose(fileArchivio);
        fclose(fileIndice);

        return 1;
    }
    


    fclose(fileArchivio);
    fclose(fileIndice);
    return 1;
}

int indexedInsertRecord(user nuovoRecord) {


    fileArchivio = fopen(ARCHIVE_FILE, "ab+");
    if (fileArchivio == NULL) {
        perror("Error opening fileArchivio");
        return 1;
    }

    fileIndice = fopen(INDEX_FILE, "ab+");
    if (fileIndice == NULL) {
        perror("Error opening fileIndice");
        return 1;
    }



    if (indexedSearchRecord(nuovoRecord.email, fileIndice) != -1) {
        fclose(fileArchivio);
        fclose(fileIndice);
        return 1;  // Record with the same key already exists
    }

    // Creation of the temporary file where all the data will be transcribed
    FILE* tempFile = fopen("tempFile", "wb");
    if (tempFile == NULL) {
        perror("Error opening temporary file");
        exit(1);
    }

    indexedUser nuovoIndexedRecord;
    indexedUser curIndexedUser;
    int pos;

    
    // Archive record
    fseek(fileArchivio, 0, SEEK_END); // Position the pointer at the end of the file
    pos = ftell(fileArchivio) / sizeof(user);// Get the position of the newly inserted record
    fwrite(&nuovoRecord, sizeof(user), 1, fileArchivio);// Insert the new record at the end of the archive

    // Index Record
    fseek(fileIndice, 0, SEEK_SET);// Position the pointer at the beginning of the file
    // Save the data to the new indexedRecord
    strcpy(nuovoIndexedRecord.email, nuovoRecord.email);
    nuovoIndexedRecord.pos = pos;


    int inserted = 0;// Record not inserted by default

    // Transcribe every record up until the moment the current record is bigger then the one we want to insert, insert the new record and then continue with the transription of each record
    while (fread(&curIndexedUser, sizeof(indexedUser), 1, fileIndice) > 0) {
        if (strcmp(nuovoRecord.email, curIndexedUser.email) < 0 && inserted == 0) {
            inserted = 1;
            fwrite(&nuovoIndexedRecord, sizeof(indexedUser), 1, tempFile);
        }
        fwrite(&curIndexedUser, sizeof(indexedUser), 1, tempFile);
    }

    // If the record wasn't inserted while in the loop it means it's in the last position.
    if (inserted == 0) {
        fwrite(&nuovoIndexedRecord, sizeof(indexedUser), 1, tempFile);
    }

    // Close each file
    fclose(fileIndice);
    fclose(tempFile);

    remove(INDEX_FILE);      // Remove the original file
    rename("tempFile", INDEX_FILE);  // Rename the temporary file making the changes effective


    fclose(fileArchivio);
    fclose(fileIndice);
    return 0;  // Record inserted successfully
}


user getUser(char email[]) {
    fileArchivio = fopen(ARCHIVE_FILE, "ab+");
    if (fileArchivio == NULL) {
        perror("Error opening fileArchivio");
        exit(1);
    }

    fileIndice = fopen(INDEX_FILE, "ab+");
    if (fileIndice == NULL) {
        perror("Error opening fileIndice");
        exit(1);
    }

    user requestedUser;
    int pos;
    pos=indexedSearchRecord(email, fileIndice);

    fseek(fileArchivio, (pos * sizeof(user)), SEEK_SET);
    fread(&requestedUser, sizeof(user), 1, fileArchivio);



    fclose(fileArchivio);
    fclose(fileIndice);
    return requestedUser;
}

int indexedDeleteRecord(char email[], FILE* fileIndice, FILE* fileArchivio) {

    if (indexedSearchRecord(email, fileIndice) == -1) return 0;// Record not found.

    // Archive file 

    user curuser;

    FILE* tempFile = fopen("tempFile", "wb");
    if (tempFile == NULL) {
        perror("Error opening temporary file");
        exit(1);
    }

    fseek(fileArchivio, 0, SEEK_SET);  // set the file position at the start of the file

    // Transcribe every record but the one that has to be deleted.
    while (fread(&curuser, sizeof(user), 1, fileArchivio) == 1) {
        if (strcmp(curuser.email, email) != 0) {
            fwrite(&curuser, sizeof(user), 1, tempFile);
        }
    }
    fclose(fileArchivio);
    fclose(tempFile);
    remove(ARCHIVE_FILE);      // Remove the original file
    rename("tempFile", ARCHIVE_FILE);  // Rename the temporary file

    // Index File

    indexedUser curIndexeduser;

    tempFile = fopen("tempFile", "wb");
    if (tempFile == NULL) {
        perror("Error opening temporary file");
        exit(1);
    }
    fseek(fileIndice, 0, SEEK_SET);  // set the file position at the start of the file

    while (fread(&curIndexeduser, sizeof(indexedUser), 1, fileIndice) == 1) {
        if (strcmp(curIndexeduser.email, email) != 0) {
            fwrite(&curIndexeduser, sizeof(indexedUser), 1, tempFile);
        }
    }
    fclose(fileIndice);
    fclose(tempFile);
    remove(INDEX_FILE);      // Remove the original file
    rename("tempFile", INDEX_FILE);  // Rename the temporary file



    return 1; // Record deleted successfully 
}