#define SVCNAME TEXT("Nat Ip Router")  

typedef void (*pMain)(char* cliMac);
typedef void (*pStop)(void);

void SvcInstall(void);
void SvcExecute(pMain startHandle,pStop stopHandle);
