#ifndef OPERAZIONIDB_H
#define OPERAZIONIDB_H

#include <stdio.h>


#define MAX_NAME_LENGTH 60
#define MAX_EMAIL_LENGTH 320
#define MAX_PASSWORD_LENGTH 128 
#define MAX_COOKIE_LENGTH 30
#define MAX_ALLOWED_FILE_SIZE 1048576
#define MAX_CONCURRENT_READERS 300
#define ARCHIVE_FILE "../DATABASE/Archivio.dat"
#define INDEX_FILE "../DATABASE/Indice.dat"
#define COOKIES_FILE "../DATABASE/IndiceCookies.dat"

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

void InitializeLibrary();
void CleanupLibrary();
void AcquireReadLock();
void ReleaseReadLock();
void AcquireWriteLock();
void ReleaseWriteLock();

int indexedSearchRecordRecursive(char email[], FILE* fileIndice, int start, int end);
int indexedSearchRecord(char email[], FILE* fileIndice);
int cookieIndexedSearchRecordRecursive(char cookie[], FILE* fileCookies, int start, int end);
int cookieIndexedSearchRecord(char cookie[], FILE* fileCookies);

int loginAuthentication(char email[], char password[]);
int indexedInsertRecord(user nuovoRecord);
user getUserByCookie(char cookie[]);
user getUserByEmail(char email[]);


#endif