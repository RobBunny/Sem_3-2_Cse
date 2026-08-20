#include <bits/stdc++.h>
using namespace std;

int main() {
    
    int n = 4;
    int m = 3;
    int Existing[m] = {9, 5, 7};
    int Possessed[n][m] = {
        {0, 1, 0},
        {2, 0, 0},
        {3, 0, 2},
        {2, 1, 1}
    };
    int Max[n][m] = {
        {7, 5, 3},
        {3, 2, 2},
        {9, 0, 2},
        {2, 2, 2}
    };
    int Available[m] = {0};
    int Req_per_res[m] = {0};
    int flag[n] = {0};
    int Workseq[n] = {0};
    int Need[n][m];
    
    for(int i=0; i<m; i++) {
        for(int j=0; j<n; j++) {
            Req_per_res[i] += Possessed[j][i];
        }
    }

    for(int i=0; i<m; i++) {
        Available[i] = Existing[i] - Req_per_res[i];
    }

    for(int i=0; i<n; i++) {
        for(int j=0; j<m; j++) {
            Need[i][j] = Max[i][j] - Possessed[i][j];
        }
    }

    cout << "Available resources" << endl; 
    for(int i=0; i<m; i++) {
        cout << Available[i] << " ";
    }
    cout << endl;

    cout << "Need for process P" << endl;
    for(int i=0; i<n; i++) {
        for(int j=0; j<m; j++) {
            cout << Need[i][j] << " ";
        }
        cout << endl;
    }

    int count = 0;
    while(count < n) {
        bool cont = false;
        for(int i=0; i<n; i++) {
            if(flag[i] == 0) {
                bool can_do = true;
                for(int j=0; j<m; j++) {
                    if(Available[j] < Need[i][j]) {
                        can_do = false;
                        break;
                    }
                }
                if(can_do) { 
                    for(int j=0; j<m; j++) {
                        Available[j] += Possessed[i][j];
                    }
                    Workseq[count] = i;
                    flag[i] = 1;
                    count++;
                    cont = true;
                }
            }
        }
        if(!cont) {
            break;
        }
    }

    if(count == n) {
        cout << "SYSTEM IS SAFE" << endl;
        cout << "SAFE SEQUENCE: ";

        for (int i = 0; i < n; i++) {
            if (i == 0) cout << "P" << Workseq[i];
            else cout << " -> " << "P" << Workseq[i];
        }
        cout << endl;
    } else {
        cout << "SYSTEM IS UNSAFE" << endl;
    }

    return 0;
}