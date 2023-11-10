#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>

#define MAX_NAME_LENGTH 60
#define MAX_EMAIL_LENGTH 320
#define MAX_PASSWORD_LENGTH 128 
#define MAX_COOKIE_LENGTH 30
#define MAX_ALLOWED_FILE_SIZE 1048576

#define ARCHIVE_FILE "../DATABASE/Archivio.dat"
#define INDEX_FILE "../DATABASE/Indice.dat"
#define COOKIES_FILE "../DATABASE/IndiceCookies.dat"


//#pragma warning(disable:4996)

typedef struct {
    char username[MAX_NAME_LENGTH];
    char email[MAX_EMAIL_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    char cookie[MAX_COOKIE_LENGTH];
} user;

typedef struct {
    char email[MAX_EMAIL_LENGTH];
    int pos;
} indexedUser;

typedef struct {
    char cookie[MAX_COOKIE_LENGTH];
    int pos;
} indexedCookie;

FILE* fileIndice;
FILE* fileArchivio;
FILE* fileCookies;


/*
semaphores structure:
    reader:
	    acquire readersem
	    release readersem
	
	
	
	    readercount++
	    if(readercount==0)
		    acquire writersem
		
	    --code
	
	    readercount--
	    if readercount==0
		    release writersem



    writer
	    acquire readersem
	    acquire writersem
	
	    --code
	
	    release readersem
	    release writersem

*/

HANDLE readSemaphore;
HANDLE writeSemaphore;
LONG readerCount = 0;


void InitializeLibrary() {
    readSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
    writeSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
}

void CleanupLibrary() {
    CloseHandle(readSemaphore);
    CloseHandle(writeSemaphore);
}

void AcquireReadLock() {
    WaitForSingleObject(readSemaphore, INFINITE);  // check if the writer is waiting


    InterlockedIncrement(&readerCount);
    if (readerCount == 1) {
        WaitForSingleObject(writeSemaphore, INFINITE);  // Prevent writers
    }

    ReleaseSemaphore(readSemaphore, 1, NULL);   // allow writer to 'book' him self
}

void ReleaseReadLock() {
    InterlockedDecrement(&readerCount);
    if (readerCount == 0) {
        ReleaseSemaphore(writeSemaphore, 1, NULL);   // allow writers
    }
}

void AcquireWriteLock() {
    WaitForSingleObject(readSemaphore, INFINITE);  // Wait for the turnstile

    WaitForSingleObject(writeSemaphore, INFINITE);  // Prevent other writers
}

void ReleaseWriteLock() {
    ReleaseSemaphore(readSemaphore, 1, NULL);    // Allow readers
    ReleaseSemaphore(writeSemaphore, 1, NULL);   // Allow other writers
}

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

int cookieIndexedSearchRecordRecursive(char cookie[], FILE* fileCookies, int start, int end) {
    // Check if the search range is valid.
    if (start <= end) {
        indexedCookie indexedCookieRecord;
        // Calculate the middle index.
        int mid = (start + end) / 2;
        // Position the file pointer at the middle of the file.
        fseek(fileCookies, mid * sizeof(indexedCookie), SEEK_SET);
        // Read one record from the file into indexedRecord.
        fread(&indexedCookieRecord, sizeof(indexedCookie), 1, fileCookies);

        // Compare the read key with the search key.
        int compare = strcmp(indexedCookieRecord.cookie, cookie);
        if (compare == 0) {
            // Return the position if the record is found.
            return indexedCookieRecord.pos; // Record trovato, restituisci la posizione
        }
        else if (compare < 0) {
            // Recursively search the right half of the range.
            return cookieIndexedSearchRecordRecursive(cookie, fileCookies, mid + 1, end);
        }
        else {
            // Recursively search the left half of the range.
            return cookieIndexedSearchRecordRecursive(cookie, fileCookies, start, mid - 1);
        }
    }
    return -1; // Record not found
}

int cookieIndexedSearchRecord(char cookie[], FILE* fileCookies) {
    // Calculate the size of the file
    fseek(fileCookies, 0, SEEK_END);
    int fileSize = ftell(fileCookies);
    int start = 0;
    int end = fileSize / sizeof(indexedCookie) - 1;

    int result = cookieIndexedSearchRecordRecursive(cookie, fileCookies, start, end);
    


    return result; // record position or -1 if not found 
}

int loginAuthentication(char email[], char password[]) {
    AcquireReadLock();
    fopen_s(&fileArchivio,ARCHIVE_FILE, "ab+");
    if (fileArchivio == NULL) {
        perror("loginAuthentication: Error opening fileArchivio");
        ReleaseReadLock();
        return 1;
    }

    fopen_s(&fileIndice,INDEX_FILE, "ab+");
    if (fileIndice == NULL) {
        perror("Error opening fileIndice");
        ReleaseReadLock();
        fclose(fileArchivio);
        return 1;
    }

    int pos;
    user curUser;

    // if not found
    pos = indexedSearchRecord(email, fileIndice);
    if (pos == -1) {
        fclose(fileArchivio);
        fclose(fileIndice);
        ReleaseReadLock();
        return 1; //email non trovata
    }


    fseek(fileArchivio, (pos * sizeof(user)),SEEK_SET);
    fread(&curUser, sizeof(user), 1, fileArchivio);
    if (strcmp(curUser.email,email)==0&&strcmp(curUser.password,password)==0) {
        fclose(fileArchivio);
        fclose(fileIndice);
        ReleaseReadLock();
        return 0; // login valido
    }else {
        fclose(fileArchivio);
        fclose(fileIndice);
        ReleaseReadLock();
        return 2; // password non valida
    }
    


    fclose(fileArchivio);
    fclose(fileIndice);
    ReleaseReadLock();
    return 3;
}

void generateCookie(char cookie[]) {
    // Seed the random number generator with the current time
    srand((unsigned int)(time(NULL)));

    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    int i;

    if (cookie) {
        for (i = 0; i < MAX_COOKIE_LENGTH - 1; i++) {
            int index = (int)(rand() % (sizeof(charset) - 1));
            cookie[i] = charset[index];
        }
        cookie[i] = '\0'; // Null-terminate the string
    }
}



int indexedInsertRecord(user nuovoRecord) {
    AcquireWriteLock();
    fopen_s(&fileIndice,INDEX_FILE, "ab+");
    if (fileIndice == NULL) {
        perror("Error opening fileIndice");
        ReleaseWriteLock();
        exit(1);
    }
    
    if (indexedSearchRecord(nuovoRecord.email, fileIndice) != -1) {
        fclose(fileIndice);
        ReleaseWriteLock();
        return 1;  // Record with the same key already exists
    }

    
    fopen_s(&fileArchivio,ARCHIVE_FILE, "ab+");
    if (fileArchivio == NULL) {
        perror("indexedInsertRecord: Error opening fileArchivio");
        ReleaseWriteLock();
        exit(1);
    }


    indexedUser nuovoIndexedRecord;
    indexedUser curIndexedUser;
    indexedCookie curIndexedCookie;
    indexedCookie nuovoIndexedCookie;
    int pos;
    int renameReturn;

    
    // Archive record
    fseek(fileArchivio, 0, SEEK_END); // Position the pointer at the end of the file
    pos = ftell(fileArchivio) / sizeof(user);// Get the position of the newly inserted record
    fwrite(&nuovoRecord, sizeof(user), 1, fileArchivio);// Insert the new record at the end of the archive
    fclose(fileArchivio);
    



    

    // Creation of the temporary file where all the data will be transcribed
    FILE* tempFile;
    fopen_s(&tempFile,"tempFile", "wb");
    if (tempFile == NULL) {
        perror("Error opening temporary file");
        ReleaseWriteLock();
        exit(1);
    }




    // Index Record
    fseek(fileIndice, 0, SEEK_SET);// Position the pointer at the beginning of the file
    // Save the data to the new indexedRecord
    strcpy_s(nuovoIndexedRecord.email, nuovoRecord.email);
    nuovoIndexedRecord.pos = pos;

    // Save the data to the new indexedCookieRecord
    strcpy_s(nuovoIndexedCookie.cookie, nuovoRecord.cookie);
    //printf("INSERITO NUOVO COOKIE: <%s>", nuovoIndexedCookie.cookie);
    nuovoIndexedCookie.pos = pos;

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
    renameReturn=rename("tempFile", INDEX_FILE);  // Rename the temporary file making the changes effective
    if (renameReturn!=0) {
        ReleaseWriteLock();
        exit(3);//rename failed
    }

    


    fopen_s(&fileCookies,COOKIES_FILE, "ab+");
    if (fileCookies == NULL) {
        perror("Error opening fileCookies");
        ReleaseWriteLock();
        exit(1);
    }

    


    // opening the temporary file where all the data will be transcribed
    fopen_s(&tempFile, "tempFile", "wb");
    if (tempFile == NULL) {
        perror("Error opening temporary file");
        ReleaseWriteLock();
        exit(1);
    }

    // Cookies Record
    fseek(fileCookies, 0, SEEK_SET);// Position the pointer at the beginning of the file
    


   inserted = 0;// Record not inserted by default

    // Transcribe every record up until the moment the current record is bigger then the one we want to insert, insert the new record and then continue with the transription of each record
    while (fread(&curIndexedCookie, sizeof(indexedCookie), 1, fileCookies) > 0) {
        if (strcmp(nuovoRecord.cookie, curIndexedCookie.cookie) < 0 && inserted == 0) {
            inserted = 1;
            fwrite(&nuovoIndexedCookie, sizeof(indexedCookie), 1, tempFile);
        }
        fwrite(&curIndexedCookie, sizeof(indexedCookie), 1, tempFile);
    }

    // If the record wasn't inserted while in the loop it means it's in the last position.
    if (inserted == 0) {
        fwrite(&nuovoIndexedCookie, sizeof(indexedCookie), 1, tempFile);
    }

    // Close each file
    fclose(fileCookies);
    fclose(tempFile);

    remove(COOKIES_FILE);      // Remove the original file
    renameReturn=rename("tempFile", COOKIES_FILE);  // Rename the temporary file making the changes effective
    if (renameReturn != 0) {
        ReleaseWriteLock();
        exit(3);//rename failed
    }

    ReleaseWriteLock();
    return 0;  // Record inserted successfully
}

// function that returns an user based on his cookie, returns a NULL user if not found.
user getUserByCookie(char cookie[]) {
    AcquireReadLock();
    fopen_s(&fileArchivio,ARCHIVE_FILE, "rb");
    if (fileArchivio == NULL) {
        perror("getUserByCookie Error opening fileArchivio");
        ReleaseReadLock();
        exit(1);
    }

    fopen_s(&fileCookies,COOKIES_FILE, "rb");
    if (fileCookies == NULL) {
        perror("Error opening fileCookies");
        fclose(fileArchivio);
        ReleaseReadLock();
        exit(1);
    }

    user requestedUser;
    requestedUser = {NULL,NULL,NULL,NULL};


    int pos;
    pos = cookieIndexedSearchRecord(cookie, fileCookies);
    if (pos == -1) {
        ReleaseReadLock();
        fclose(fileArchivio);
        fclose(fileCookies);
        return requestedUser;//user not found
    }

    fseek(fileArchivio, (pos * sizeof(user)), SEEK_SET);
    fread(&requestedUser, sizeof(user), 1, fileArchivio);



    fclose(fileArchivio);
    fclose(fileCookies);
    ReleaseReadLock();
    return requestedUser;
}

// function that returns an user based on his email, returns an initialized user if not found.
user getUserByEmail(char email[]) {
    AcquireReadLock();
    fopen_s(&fileArchivio,ARCHIVE_FILE, "rb");
    if (fileArchivio == NULL) {
        perror("getUserByEmail: Error opening fileArchivio");
        ReleaseReadLock();
        exit(1);
    }

    fopen_s(&fileIndice,INDEX_FILE, "rb");
    if (fileIndice == NULL) {
        perror("Error opening fileIndice");
        ReleaseReadLock();
        fclose(fileArchivio);
        exit(1);
    }

    user requestedUser;
    requestedUser = { NULL,NULL,NULL,NULL };
    int pos;
    pos = indexedSearchRecord(email, fileIndice);
    if (pos==-1) {
        ReleaseReadLock();
        fclose(fileArchivio);
        fclose(fileIndice);
        return requestedUser;// user not found
    }

    fseek(fileArchivio, (pos * sizeof(user)), SEEK_SET);
    fread(&requestedUser, sizeof(user), 1, fileArchivio);



    fclose(fileArchivio);
    fclose(fileIndice);
    ReleaseReadLock();
    return requestedUser;
}




// To implement.
/*
int indexedDeleteRecord(char email[], FILE* fileIndice, FILE* fileArchivio) {
    if (indexedSearchRecord(email, fileIndice) == -1) return 0;// Record not found.

    // Archive file 

    user curuser;
    int renameReturn;

    FILE* tempFile;
    fopen_s(&tempFile, "tempFile", "wb");
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
    renameReturn=rename("tempFile", ARCHIVE_FILE);  // Rename the temporary file
    if (!renameReturn) {
        exit(3);//rename failed
    }
    // Index File

    indexedUser curIndexeduser;

    fopen_s(&tempFile, "tempFile", "wb");
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
    renameReturn=rename("tempFile", INDEX_FILE);  // Rename the temporary file
    if (!renameReturn) {
        exit(3);//rename failed
    }


    return 1; // Record deleted successfully 
}
*/