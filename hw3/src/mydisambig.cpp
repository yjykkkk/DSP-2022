#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <float.h>
#include <string>
#include <algorithm>
#include <sstream>
#include "Ngram.h"
#define BUFFSIZE 10000

using namespace std;
map<string, vector<string>> mapping;
vector <vector <string>> text;
vector <vector<string>> word_table;  
vector <vector<double>> prob_table;
vector <vector<int>> backtrack;
int order = 2;
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

int main(int argc, char **argv){
    //freopen("test_answer","w",stdout);
    //segemented file to be decoded = argv[1];
    //ZhuYin-Big5 mapping = argv[2];
    //language model = argv[3];
    //output file = argv[4];
    File lmFile(argv[3], "r"); 
    lm.read(lmFile);
    lmFile.close();
    //create mapping
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
    int curr;
    int next;
    while(textFile.getline(buf2, sizeof(buf2))){
        next = 0;
        curr = 0;
        seq.clear();
        string s = buf2;
        while (1){
            next = s.find_first_of(" \n", curr); 
            if (next == -1){
                break;
            }
            if (next != curr){
                w = s.substr(curr, next - curr);
                if (!w.empty()){
                    seq.push_back(w);
                }
            }
            curr = next + 1;
        }
        text.push_back(seq);
    }
    textFile.close();

    ofstream outfile;
    outfile.open(argv[4]); 
    
    vector <string> word;
    vector <double> prob;
    vector <int> back;

    for (int sequence=0; sequence<text.size(); sequence++){
        word_table.clear();
        prob_table.clear();
        backtrack.clear();
        
        map<string, vector<string>>::iterator iter;
        for (int t=0; t<text[sequence].size(); t++){
            iter = mapping.find(text[sequence][t]);
            word.clear();
            prob.clear();
            back.clear();
            if (iter != mapping.end()){ //找得到
                for (int i=0; i<iter->second.size(); i++){
                    word.push_back(iter->second[i]);
                    prob.push_back(0);
                    back.push_back(0);
                }
            }
            else{   
                word.push_back(text[sequence][t].c_str());
                prob.push_back(0);
                back.push_back(0);
            }
            word_table.push_back(word);
            prob_table.push_back(prob);
            backtrack.push_back(back);
        }

        double max_prob; //delta_t(i)
        double curr_prob;
        int best_j;
        for (int t=0; t<word_table.size(); t++){
            if (t==0){
                for (int i=0; i<word_table[t].size(); i++){
                    curr_prob = getBigramProb("<s>", word_table[t][i].c_str());
                    prob_table[t][i] = curr_prob;
                    //backtrack[t][i] = best_j;
                }
            }
            else{
                for (int i=0; i<word_table[t].size(); i++){
                    max_prob = -DBL_MAX;    //since log value may be negative
                    for (int j=0; j<word_table[t-1].size(); j++){   //前一個 word
                        curr_prob = prob_table[t-1][j];
                        curr_prob += getBigramProb(word_table[t-1][j].c_str(), word_table[t][i].c_str());
                        if (curr_prob > max_prob){
                            max_prob = curr_prob;
                            best_j = j;
                        }
                    }
                    prob_table[t][i] = max_prob;
                    backtrack[t][i] = best_j;
                }
            }
        }
        int T = word_table.size();
        prob.clear();
        max_prob = -DBL_MAX;
        for (int j=0; j<word_table[T-1].size(); j++){
            curr_prob = getBigramProb(word_table[T-1][j].c_str(), "</s>");
            curr_prob += prob_table[T-1][j];
            if (curr_prob > max_prob){
                max_prob = curr_prob;
                best_j = j;
            }
        }
        
        vector<string> ans;
        double P_star; //max prob of T
        int q_T_star; // the state that have max prob of T 
        max_prob = -DBL_MAX;
        
        int now_best = best_j;
        for (int t=T-1; t>=0; t--){
            ans.push_back(word_table[t][now_best]);
            if (t != 0)
                now_best = backtrack[t][now_best];
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
