#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <stack>
#include <float.h>
#include <string>
#include <algorithm>
#include <sstream>
#include <stack>
#include "Ngram.h"
#define BUFFSIZE 10000
#define THRESHOLD 200

using namespace std;
map<string, vector<string>> mapping;
vector <vector <string>> text;
vector< vector <vector <double>>> prob_table;   //delta_t(i, j)
vector< vector <vector <int>>> backtrack;   //psi_t(i, j)
int order = 3;
vector <string> cands; //candidates
map<string, vector<string>>::iterator iter;
//read language model
Vocab voc;
Ngram lm(voc, order);

// Get P(W2 | W1) -- bigram
double getBigramProb(const char *w1, const char *w2)
{
    VocabIndex wid1 = voc.getIndex(w1);
    VocabIndex wid2 = voc.getIndex(w2);

    if(wid1 == Vocab_None)  //OOV
        wid1 = voc.getIndex(Vocab_Unknown);
    if(wid2 == Vocab_None)  //OOV
        wid2 = voc.getIndex(Vocab_Unknown);

    VocabIndex context[] = { wid1, Vocab_None };
    return lm.wordProb( wid2, context);
}

// Get P(W3 | W1, W2) -- trigram
double getTrigramProb(const char *w1, const char *w2, const char *w3) 
{
    VocabIndex wid1 = voc.getIndex(w1);
    VocabIndex wid2 = voc.getIndex(w2);
    VocabIndex wid3 = voc.getIndex(w3);

    if(wid1 == Vocab_None)  //OOV
        wid1 = voc.getIndex(Vocab_Unknown);
    if(wid2 == Vocab_None)  //OOV
        wid2 = voc.getIndex(Vocab_Unknown);
    if(wid3 == Vocab_None)  //OOV
        wid3 = voc.getIndex(Vocab_Unknown);

    VocabIndex context[] = { wid2, wid1, Vocab_None };
    return lm.wordProb( wid3, context );
}

void getcands(const char* w, vector<string>& v){
    v.clear();
    iter = mapping.find(w);
    if (iter == mapping.end()){
        v.push_back(w);
    }
    else{
        for (int i=0; i<iter->second.size(); i++){
            v.push_back(iter->second[i]);
        }
    }
}
struct Compare {
    Compare(Vocab* voc,  Ngram* lm) { this->voc = voc; this->lm = lm;}
    bool operator () (string i, string j) {
        VocabIndex ci = voc->getIndex(i.c_str());
        VocabIndex cj = voc->getIndex(j.c_str());
        if (ci == Vocab_None){
            ci = voc->getIndex(Vocab_Unknown);
        }
        if (cj == Vocab_None){
            cj = voc->getIndex(Vocab_Unknown);
        }
        VocabIndex context[] = {Vocab_None};
        return lm->wordProb(ci, context) > lm->wordProb(cj, context);
    }
    Vocab* voc;
    Ngram* lm;
};

int main(int argc, char **argv){
    File lmFile(argv[3], "r"); 
    lm.read(lmFile);
    lmFile.close();

    //create ZhuYin-Big5 mapping
    char buf[BUFFSIZE];
    fstream Mapfile;
    Mapfile.open(argv[2], ios::in); 
    string buffer;
    string ZhuYin;
    string Chinese;
    while(Mapfile.getline(buf, sizeof(buf))){
        buffer = string(buf);
        ZhuYin = buffer.substr(0, buffer.find("\t"));
        vector<string> ch_vec;
        buffer = buffer.substr(buffer.find("\t")+1, buffer.length());
        while (buffer.find(" ")!=-1){
            Chinese = buffer.substr(0, buffer.find(" "));
            ch_vec.push_back(Chinese);
            buffer = buffer.substr(buffer.find(" ")+1, buffer.length());            
        }
        Chinese = buffer;
        ch_vec.push_back(Chinese);
        mapping.insert(pair<string, vector<string>>(ZhuYin, ch_vec));
    } 
    Mapfile.close();

    //read segemented file to be decoded
    char buf2[BUFFSIZE];
    fstream textFile;
    textFile.open(argv[1], ios::in);
    vector <string> seq;
    string w;
    int current;
    int next;
    while(textFile.getline(buf2, sizeof(buf2))){
        next = 0;
        current = 0;
        seq.clear();
        string s = buf2;
        while (1){
            next = s.find_first_of(" \n", current); 
            if (next == -1){
                break;
            }
            if (next != current){
                w = s.substr(current, next - current);
                if (!w.empty()){
                    seq.push_back(w);
                }
            }
            current = next + 1;
        }
        text.push_back(seq);
    }
    textFile.close();

    ofstream outfile;
    outfile.open(argv[4]); 
    
    vector <double> prob;
    vector <int> back;
    vector <vector<double>> prob_vec;
    vector <vector<int>> backtrack_vec;
    int i_len, j_len;
    vector <vector<string>> words;  

    for (int sequence=0; sequence<text.size(); sequence++){
        words.clear();

        prob_table.clear();
        backtrack.clear();

        prob_vec.clear();
        backtrack_vec.clear();

        prob.clear();
        back.clear();

        int cand_count; 
        for (int t=0; t<text[sequence].size(); t++){
            prob_vec.clear();
            backtrack_vec.clear();
            prob.clear();
            back.clear();
            getcands(text[sequence][t].c_str(), cands);
            sort(cands.begin(), cands.end(), Compare(&voc, &lm));
            cand_count = cands.size();
            if (cand_count >= THRESHOLD){
                cands.erase(cands.begin() + THRESHOLD, cands.end());
                cand_count = THRESHOLD;
            }
            words.push_back(cands);
            if (t==0){
                prob.clear();
                for (int i=0; i<cand_count; i++){
                    //prob.push_back(getBigramProb("<s>", cands[i].c_str()));
                    back.push_back(0);
                    prob.push_back(0);
                }
                prob_vec.push_back(prob);
                backtrack_vec.push_back(back);
            }  
            else{
                for (int i=0; i<words[t].size(); i++){
                    prob.clear();
                    back.clear();
                    for (int j=0; j<words[t-1].size(); j++){
                        prob.push_back(0);
                        back.push_back(0);
                    }
                    prob_vec.push_back(prob);
                    backtrack_vec.push_back(back);
                }
            }
            prob_table.push_back(prob_vec);
            backtrack.push_back(backtrack_vec);
        }
        
        int best_j, best_k;
        double max_prob; //delta_t(i)
        double curr_prob;
        for (int t=0; t<text[sequence].size(); t++){
            if (t==0){
                for (int i=0; i<words[t].size(); i++){
                    curr_prob = getBigramProb("<s>", words[t][i].c_str());         
                    prob_table[t][0][i] = curr_prob;
                }
            }
            else if (t==1){
                for (int i=0; i<words[t].size(); i++){
                    for (int j=0; j<words[t-1].size(); j++){
                        curr_prob = prob_table[t-1][0][j];
                        curr_prob += getTrigramProb("<s>", words[t-1][j].c_str(), words[t][i].c_str());
                        prob_table[t][i][j] = curr_prob;
                        backtrack[t][i][j] = -1; //not used
                    }
                }
            }
            else{
                for (int i=0; i<words[t].size(); i++){
                    max_prob = -DBL_MAX;
                    for (int j=0; j<words[t-1].size(); j++){
                        max_prob = -DBL_MAX;
                        for (int k=0; k<words[t-2].size(); k++){ 
                            curr_prob = prob_table[t-1][j][k];
                            curr_prob += getTrigramProb(words[t-2][k].c_str(), words[t-1][j].c_str(), words[t][i].c_str());
                            if (curr_prob > max_prob){
                                max_prob = curr_prob;
                                best_k = k;
                            }
                        }
                        prob_table[t][i][j] = max_prob;
                        backtrack[t][i][j] = best_k;
                    }
                }
            }
        }
        
        prob_vec.clear();
        backtrack_vec.clear();
        prob.clear();
        back.clear();        
        max_prob = -DBL_MAX;
        int T = words.size();
        for (int j=0; j<words[T-1].size(); j++){
            max_prob = -DBL_MAX;
            for (int k=0; k<words[T-2].size(); k++){ 
                curr_prob = prob_table[T-1][j][k];
                curr_prob += getTrigramProb(words[T-2][k].c_str(), words[T-1][j].c_str(), "</s>");
                if (curr_prob > max_prob){
                    max_prob = curr_prob;
                    best_k = k;
                }
            }
            prob.push_back(max_prob);
            back.push_back(best_k);
            //prob_table[t][i][j] = max_prob;
            //backtrack[t][i][j] = best_k;
        }
        prob_vec.push_back(prob);
        backtrack_vec.push_back(back);
        prob_table.push_back(prob_vec);
        backtrack.push_back(backtrack_vec);
        
        vector<string> ans;
        vector<double> final_prob;
        double P_star; //max prob of T
        int now_best; // the state that have max prob of T 
        int prev_best;
        
        max_prob = -DBL_MAX;
        for (int j=0; j<prob_table[T][0].size(); j++){
            if (prob_table[T][0][j] > max_prob){
                max_prob = prob_table[T][0][j];
                now_best = j;
            }
        }        

        max_prob = -DBL_MAX;
        for (int j=0; j<words[T-2].size(); j++){
            curr_prob = prob_table[T-1][now_best][j];
            if (curr_prob > max_prob){
                max_prob = curr_prob;
                prev_best = j;
            }
        }
        int track = backtrack[T-1][now_best][prev_best]; //倒數第三個字
        //printf("endword has %d cands, and %d \n", words[T-1].size(), prob_table[T][0].size());
        //printf("last last word has %d cands, and %d \n", words[T-2].size(), prob_table[T-1][now_best].size());
        //printf("track = %d\n", track);
        for (int t=T-1; t>=0; t--){
            ans.push_back(words[t][now_best]);
            now_best = prev_best;
            prev_best = track;            
            if (t != 1 && t != 0)
                track = backtrack[t-1][now_best][prev_best];
        }
        
        string answer = "<s> ";
        int total = ans.size();
        for (int i=total-1; i>=0; i--){
            answer += ans[i];
            answer += " ";
        }
        answer += "</s>";        
        outfile<<answer<<endl;        
    }
    outfile.close();
}

