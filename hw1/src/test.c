#include "hmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int seq_num = 2500;
//int model_num = 5;
double delta[5][2500][MAX_SEQ][MAX_STATE];
int psi[5][2500][MAX_SEQ];//max_i for backtracking
double p_star[5][2500];//max prob of T-1
char model_name[5][30];

int accuracy(FILE *testing_result_file, FILE *ground_truth_file){
    char prediction[MAX_LINE], truth[MAX_LINE];
    int correct_samples = 0;
    while(fgets(prediction, MAX_LINE, testing_result_file) != NULL && fgets(truth, MAX_LINE, ground_truth_file) != NULL){    
        if(strcmp(prediction, truth) == 0) 
            correct_samples++;
    }
    return correct_samples;
}

int main(int argc, char **argv){
    char seq[seq_num][MAX_SEQ];
    FILE *fp = open_or_die(argv[2], "r"); //seq_path
    for (int i=0; i<seq_num; i++){
        fscanf(fp, "%s", seq[i]);
    }
    FILE *fp_result = open_or_die(argv[3], "w"); //output_result_path
    int T = strlen(seq[0]);
    FILE *fp_list = open_or_die(argv[1], "r"); //models_list_path
    int model_num = 0;
    while(fscanf(fp_list, "%s", model_name[model_num])>0)
        model_num++;
    HMM model[model_num];
    load_models(argv[1], model, model_num);
    //dump_models(model, 5);
    for (int sequence=0; sequence<seq_num; sequence++){
        for (int model_count=0; model_count<model_num; model_count++){
            for (int t=0; t<T; t++){
                for (int state=0; state<model[model_count].state_num; state++){
                    if (t==0){//delta_1(i) = pi_i*b_i(o_1)
                        delta[model_count][sequence][0][state] = model[model_count].initial[state] * model[model_count].observation[seq[sequence][0]-'A'][state];
                    }
                    else{
                        double maxsum = 0.0;
                        double cursum;
                        for (int i=0; i<model[model_count].state_num; i++){
                            cursum = delta[model_count][sequence][t-1][i] * model[model_count].transition[i][state];
                            if (cursum>maxsum){
                                maxsum = cursum;
                                psi[model_count][sequence][t] = i;
                            }
                        }
                        int o_time = seq[sequence][t]-'A';
                        delta[model_count][sequence][t][state] = maxsum * model[model_count].observation[o_time][state];
                    }
                }
            }      
            //find max prob of T-1 
            p_star[model_count][sequence] = 0;
            for (int i=0; i<model[model_count].state_num; i++){
                if (delta[model_count][sequence][T-1][i] > p_star[model_count][sequence]){
                    p_star[model_count][sequence] = delta[model_count][sequence][T-1][i];
                }
            }
        }
        double maxprob = 0;
        int maxid;
        for (int model_id=0; model_id<model_num; model_id++){
            if (p_star[model_id][sequence] > maxprob){
                maxprob = p_star[model_id][sequence];
                maxid = model_id;
            }
        }
        fprintf(fp_result, "%s %g\n", model_name[maxid], maxprob);
    }
    fclose(fp);
    fclose(fp_result);
    fclose(fp_list);
}