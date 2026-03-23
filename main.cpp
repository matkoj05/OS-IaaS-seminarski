#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <map>
using namespace std;

struct ServerCvor {
    int id;
    char naziv[50];
    ServerCvor* sledeci;
};

struct VMCvor {
    int id;
    char ime[50];
    char os[50];
    int ram;
    int cpu;
    int serverId;
    char korisnik[50];
    bool radi;           // Status VM-a (pokrenut/zaustavljen)
    char ip[20];         // IP adresa
    int diskSize;        // Veličina diska u GB
    VMCvor* sledeci;
};

ServerCvor* serverLista = nullptr;
VMCvor* vmLista = nullptr;
char trenutniKorisnik[50] = "";
int sledeciIP = 100;     // Početna IP adresa za dodeljivanje

//////////////////////////////////////////////////////////
// FAJLOVI
//////////////////////////////////////////////////////////

void procitajServereIzFajla() {
    FILE* f = fopen("serveri.dat", "rb");
    if (!f) {
        // Podrazumevani serveri
        ServerCvor* novi = new ServerCvor;
        novi->id = 1; strcpy(novi->naziv, "AWS");
        novi->sledeci = serverLista; serverLista = novi;
        
        novi = new ServerCvor;
        novi->id = 2; strcpy(novi->naziv, "Google Cloud");
        novi->sledeci = serverLista; serverLista = novi;
        
        novi = new ServerCvor;
        novi->id = 3; strcpy(novi->naziv, "Azure");
        novi->sledeci = serverLista; serverLista = novi;
        return;
    }
    
    ServerCvor temp;
    while (fread(&temp, sizeof(ServerCvor) - sizeof(ServerCvor*), 1, f)) {
        ServerCvor* novi = new ServerCvor;
        novi->id = temp.id;
        strcpy(novi->naziv, temp.naziv);
        novi->sledeci = serverLista;
        serverLista = novi;
    }
    fclose(f);
}

void upisiServereUFajl() {
    FILE* f = fopen("serveri.dat", "wb");
    if (!f) return;
    
    ServerCvor* trenutni = serverLista;
    while (trenutni) {
        fwrite(trenutni, sizeof(ServerCvor) - sizeof(ServerCvor*), 1, f);
        trenutni = trenutni->sledeci;
    }
    fclose(f);
}

void procitajVMoveIzFajla() {
    FILE* f = fopen("vmovi.dat", "rb");
    if (!f) return;
    
    VMCvor temp;
    while (fread(&temp, sizeof(VMCvor) - sizeof(VMCvor*), 1, f)) {
        VMCvor* novi = new VMCvor;
        novi->id = temp.id;
        strcpy(novi->ime, temp.ime);
        strcpy(novi->os, temp.os);
        novi->ram = temp.ram;
        novi->cpu = temp.cpu;
        novi->serverId = temp.serverId;
        strcpy(novi->korisnik, temp.korisnik);
        novi->radi = temp.radi;
        strcpy(novi->ip, temp.ip);
        novi->diskSize = temp.diskSize;
        novi->sledeci = vmLista;
        vmLista = novi;
    }
    fclose(f);
}

void upisiVMoveUFajl() {
    FILE* f = fopen("vmovi.dat", "wb");
    if (!f) return;
    
    VMCvor* trenutni = vmLista;
    while (trenutni) {
        fwrite(trenutni, sizeof(VMCvor) - sizeof(VMCvor*), 1, f);
        trenutni = trenutni->sledeci;
    }
    fclose(f);
}

//////////////////////////////////////////////////////////
// LOGIN
//////////////////////////////////////////////////////////

struct Korisnik {
    char username[50];
    char password[50];
};

bool proveriLogin(const char* username, const char* password) {
    FILE* f = fopen("korisnici.dat", "rb");
    if (!f) {
        f = fopen("korisnici.dat", "wb");
        Korisnik admin = {"admin", "admin123"};
        fwrite(&admin, sizeof(Korisnik), 1, f);
        fclose(f);
        return false;
    }
    
    Korisnik temp;
    while (fread(&temp, sizeof(Korisnik), 1, f)) {
        if (strcmp(temp.username, username) == 0 && 
            strcmp(temp.password, password) == 0) {
            fclose(f);
            return true;
        }
    }
    fclose(f);
    return false;
}

bool registrujKorisnika(const char* username, const char* password) {
    FILE* f = fopen("korisnici.dat", "rb");
    if (f) {
        Korisnik temp;
        while (fread(&temp, sizeof(Korisnik), 1, f)) {
            if (strcmp(temp.username, username) == 0) {
                fclose(f);
                return false;
            }
        }
        fclose(f);
    }
    
    f = fopen("korisnici.dat", "ab");
    if (!f) return false;
    
    Korisnik novi;
    strcpy(novi.username, username);
    strcpy(novi.password, password);
    fwrite(&novi, sizeof(Korisnik), 1, f);
    fclose(f);
    return true;
}

//////////////////////////////////////////////////////////
// IaaS FUNKCIJE
//////////////////////////////////////////////////////////

void prikaziServere() {
    cout << "\n=== SERVERI ===\n";
    ServerCvor* trenutni = serverLista;
    while (trenutni) {
        cout << trenutni->id << ". " << trenutni->naziv << "\n";
        trenutni = trenutni->sledeci;
    }
}

void prikaziMojeVMove() {
    cout << "\n=== MOJI VM-OVI ===\n";
    VMCvor* trenutni = vmLista;
    bool ima = false;
    
    while (trenutni) {
        if (strcmp(trenutni->korisnik, trenutniKorisnik) == 0) {
            cout << "ID:" << trenutni->id << " " << trenutni->ime 
                 << " [" << trenutni->os << " " << trenutni->ram 
                 << "GB RAM " << trenutni->cpu << "CPU "
                 << trenutni->diskSize << "GB DISK]\n";
            cout << "   Status: " << (trenutni->radi ? "POKRENUT" : "ZAUSTAVLJEN") 
                 << " | IP: " << (strlen(trenutni->ip) > 0 ? trenutni->ip : "NEMA") << "\n";
            ima = true;
        }
        trenutni = trenutni->sledeci;
    }
    if (!ima) cout << "Nema VM-ova.\n";
}

void dodajVM(const char* ime, const char* os, int ram, int cpu, int serverId, int diskSize) {
    // Nađi sledeći ID
    int maxId = 0;
    VMCvor* trenutni = vmLista;
    while (trenutni) {
        if (trenutni->id > maxId) maxId = trenutni->id;
        trenutni = trenutni->sledeci;
    }
    
    VMCvor* novi = new VMCvor;
    novi->id = maxId + 1;
    strcpy(novi->ime, ime);
    strcpy(novi->os, os);
    novi->ram = ram;
    novi->cpu = cpu;
    novi->serverId = serverId;
    strcpy(novi->korisnik, trenutniKorisnik);
    novi->radi = false; // Podrazumevano zaustavljen
    strcpy(novi->ip, ""); // Nema IP na početku
    novi->diskSize = diskSize;
    novi->sledeci = vmLista;
    vmLista = novi;
    
    upisiVMoveUFajl();
    cout << "VM kreiran \n";
}

void obrisiVM(int id) {
    VMCvor* trenutni = vmLista;
    VMCvor* prethodni = nullptr;
    
    while (trenutni) {
        if (trenutni->id == id && strcmp(trenutni->korisnik, trenutniKorisnik) == 0) {
            if (prethodni) prethodni->sledeci = trenutni->sledeci;
            else vmLista = trenutni->sledeci;
            
            delete trenutni;
            upisiVMoveUFajl();
            cout << "VM obrisan.\n";
            return;
        }
        prethodni = trenutni;
        trenutni = trenutni->sledeci;
    }
    cout << "VM nije pronadjen.\n";
}

VMCvor* nadjiVMpoID(int id) {
    VMCvor* trenutni = vmLista;
    while (trenutni) {
        if (trenutni->id == id && strcmp(trenutni->korisnik, trenutniKorisnik) == 0) {
            return trenutni;
        }
        trenutni = trenutni->sledeci;
    }
    return nullptr;
}

void pokreniVM(int id) {
    VMCvor* vm = nadjiVMpoID(id);
    if (!vm) {
        cout << "VM nije pronadjen.\n";
        return;
    }
    
    if (vm->radi) {
        cout << "VM je vec pokrenut.\n";
        return;
    }
    
    vm->radi = true;
    
    // Ako nema IP, dodeli novu
    if (strlen(vm->ip) == 0) {
        char ip[20];
        sprintf(ip, "192.168.1.%d", sledeciIP++);
        strcpy(vm->ip, ip);
        cout << "Dodeljena IP adresa: " << vm->ip << "\n";
    }
    
    upisiVMoveUFajl();
    cout << "VM '" << vm->ime << "' je pokrenut.\n";
}

void zaustaviVM(int id) {
    VMCvor* vm = nadjiVMpoID(id);
    if (!vm) {
        cout << "VM nije pronadjen.\n";
        return;
    }
    
    if (!vm->radi) {
        cout << "VM je vec zaustavljen.\n";
        return;
    }
    
    vm->radi = false;
    upisiVMoveUFajl();
    cout << "VM '" << vm->ime << "' je zaustavljen.\n";
}

void dodeliIP(int id) {
    VMCvor* vm = nadjiVMpoID(id);
    if (!vm) {
        cout << "VM nije pronadjen.\n";
        return;
    }
    
    if (strlen(vm->ip) > 0) {
        cout << "VM vec ima IP: " << vm->ip << "\n";
        cout << "Zelite li da promenite? (d/n): ";
        char odg;
        cin >> odg;
        if (odg != 'd' && odg != 'D') return;
    }
    
    cout << "Unesite IP adresu (format: xxx.xxx.xxx.xxx): ";
    char ip[20];
    cin >> ip;
    
    strcpy(vm->ip, ip);
    upisiVMoveUFajl();
    cout << "IP adresa '" << ip << "' dodeljena VM-u '" << vm->ime << "'.\n";
}

void promeniDisk(int id) {
    VMCvor* vm = nadjiVMpoID(id);
    if (!vm) {
        cout << "VM nije pronadjen.\n";
        return;
    }
    
    cout << "Trenutna velicina diska: " << vm->diskSize << "GB\n";
    cout << "Nova velicina diska (GB): ";
    int novaVelicina;
    cin >> novaVelicina;
    
    if (novaVelicina <= 0) {
        cout << "Nevalidna velicina.\n";
        return;
    }
    
    vm->diskSize = novaVelicina;
    upisiVMoveUFajl();
    cout << "Velicina diska promenjena na " << novaVelicina << "GB.\n";
}

//////////////////////////////////////////////////////////
// GLAVNI PROGRAM
//////////////////////////////////////////////////////////

int main() {
    // Učitaj podatke
    procitajServereIzFajla();
    procitajVMoveIzFajla();
    
    // Login
    int izbor;
    char user[50], pass[50];
    
    while (true) {
        cout << "\n=== IaaS Prijava ===\n";
        cout << "1. Prijava\n2. Registracija\nIzbor: ";
        cin >> izbor;
        
        if (izbor == 1) {
            cout << "Korisnicko ime: "; cin >> user;
            cout << "Lozinka: "; cin >> pass;
            
            if (proveriLogin(user, pass)) {
                strcpy(trenutniKorisnik, user);
                break;
            }
            cout << "Greska pri prijavi.\n";
        }
        else if (izbor == 2) {
            cout << "Novo korisnicko ime: "; cin >> user;
            cout << "Nova lozinka: "; cin >> pass;
            
            if (registrujKorisnika(user, pass)) {
                strcpy(trenutniKorisnik, user);
                break;
            }
            cout << "Korisnik vec postoji.\n";
        }
    }
    
    cout << "\nDobrodosli " << trenutniKorisnik << "!\n";
    
    // Glavni meni
    do {
        cout << "\n=== IaaS MENI ===\n";
        cout << "1. Prikazi servere\n2. Prikazi moje VM-ove\n3. Kreiraj VM\n";
        cout << "4. Obrisi VM\n5. Pokreni VM\n6. Zaustavi VM\n";
        cout << "7. Dodeli IP adresu\n8. Promeni velicinu diska\n0. Izlaz\nIzbor: ";
        cin >> izbor;
        
        if (izbor == 1) {
            prikaziServere();
        }
        else if (izbor == 2) {
            prikaziMojeVMove();
        }
        else if (izbor == 3) {
            prikaziServere();
            int serverId;
            char ime[50], os[50];
            int ram, cpu, disk;
            
            cout << "Server ID: "; cin >> serverId;
            cout << "Ime VM: "; cin.ignore(); cin.getline(ime, 50);
            cout << "OS: "; cin.getline(os, 50);
            cout << "RAM (GB): "; cin >> ram;
            cout << "CPU: "; cin >> cpu;
            cout << "Velicina diska (GB): "; cin >> disk;
            
            dodajVM(ime, os, ram, cpu, serverId, disk);
        }
        else if (izbor == 4) {
            prikaziMojeVMove();
            int id;
            cout << "ID VM za brisanje: "; cin >> id;
            obrisiVM(id);
        }
        else if (izbor == 5) {
            prikaziMojeVMove();
            int id;
            cout << "ID VM za pokretanje: "; cin >> id;
            pokreniVM(id);
        }
        else if (izbor == 6) {
            prikaziMojeVMove();
            int id;
            cout << "ID VM za zaustavljanje: "; cin >> id;
            zaustaviVM(id);
        }
        else if (izbor == 7) {
            prikaziMojeVMove();
            int id;
            cout << "ID VM za dodelu IP: "; cin >> id;
            dodeliIP(id);
        }
        else if (izbor == 8) {
            prikaziMojeVMove();
            int id;
            cout << "ID VM za promenu diska: "; cin >> id;
            promeniDisk(id);
        }
    } while (izbor != 0);
    
    // Sačuvaj i oslobodi memoriju
    upisiServereUFajl();
    upisiVMoveUFajl();
    
    // Oslobodi memoriju
    while (serverLista) {
        ServerCvor* temp = serverLista;
        serverLista = serverLista->sledeci;
        delete temp;
    }
    while (vmLista) {
        VMCvor* temp = vmLista;
        vmLista = vmLista->sledeci;
        delete temp;
    }
    
    cout << "\nHvala sto koristite IaaS sistem!\n";
    return 0;
}