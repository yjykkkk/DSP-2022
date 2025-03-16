#include "hmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int seq_num = 10000;
double alpha[10000][MAX_SEQ][MAX_STATE];
double beta[10000][MAX_SEQ][MAX_STATE];
double gamma[10000][MAX_SEQ][MAX_STATE];
double epsilon[10000][MAX_SEQ][MAX_STATE][MAX_STATE];

int main(int argc, char **argv){
    int iter = atoi(argv[1]);
    char seq[10000][MAX_SEQ];
    FILE *fp = open_or_die(argv[3], "r");//seq_path argv[3]
    for (int i=0; i<10000; i++){
        fscanf(fp, "%s", seq[i]);
    }
    HMM model;
    loadHMM(&model, argv[2]); //model_init_path argv[2]
    int T = strlen(seq[0]);
    while (iter){
        for (int sequence=0; sequence<seq_num; sequence++){
            //alpha
            for (int t=0; t<T; t++){
                for (int state=0; state<model.state_num; state++){
                    if (t==0){
                        alpha[sequence][0][state] = model.initial[state]*model.observation[seq[sequence][0]-'A'][state];
                    }
                    else {
                        double sum = 0.0;//sum of alpha_t(i) * a_ij
                        for (int i=0; i<model.state_num; i++){
                            sum += alpha[sequence][t-1][i] * model.transition[i][state];
                        }
                        int o_time = seq[sequence][t]-'A';
                        alpha[sequence][t][state] = sum * model.observation[o_time][state];
                    }
                }
            }
            //beta
            for (int t=T-1; t>=0; t--){
                for (int state=0; state<model.state_num; state++){
                    if (t==T-1){
                        beta[sequence][t][state] = 1;
                    }
                    else{
                        double sum = 0.0;//sum of a_ij, b_j(o_t+1), beta_t+1(j)
                        int o_time = seq[sequence][t+1]-'A';
                        for (int j=0; j<model.state_num; j++){
                            sum += model.transition[state][j] * model.observation[o_time][j] * beta[sequence][t+1][j];
                        }
                        beta[sequence][t][state] = sum;
                    }
                }
            }
            //gamma
            for (int t=0; t<T; t++){
                double sum = 0.0; //sum of alpha_t(i)*beta_t(i)
                for (int i=0; i<model.state_num; i++){
                    sum += alpha[sequence][t][i]*beta[sequence][t][i];
                }
                for (int state=0; state<model.state_num; state++){
                    gamma[sequence][t][state] = alpha[sequence][t][state]*beta[sequence][t][state]/sum;
                }
            }
            //epsilon
            for (int t=0; t<T-1; t++){
                double sum = 0.0;
                int o_time = seq[sequence][t+1]-'A';
                for (int i=0; i<model.state_num; i++){
                    for (int j=0; j<model.state_num; j++){
                        sum += alpha[sequence][t][i] * model.transition[i][j] * model.observation[o_time][j] * beta[sequence][t+1][j];
                    }
                }
                for (int i=0; i<model.state_num; i++){
                    for (int j=0; j<model.state_num; j++){
                        epsilon[sequence][t][i][j] = alpha[sequence][t][i] * model.transition[i][j] * model.observation[o_time][j] * beta[sequence][t+1][j] / sum;
                    }
                }
            }
        }
        //---------update hmm's parameters--------------
        //initial
        for (int i=0; i<model.state_num; i++){
            double sum = 0.0;
            for (int n=0; n<10000; n++){
                sum += gamma[n][0][i];
            }
            model.initial[i] = sum/10000;
        }
        //transition
        for (int i=0; i<model.state_num; i++){
            double down_sum = 0.0;//sum of gamma
            for (int n=0; n<10000; n++){
                for (int t=0; t<T-1; t++){
                    down_sum += gamma[n][t][i];
                }
            }
            for (int j=0; j<model.state_num; j++){
                double up_sum = 0.0;//sum of epsilon
                for (int n=0; n<10000; n++){
                    for (int t=0; t<T-1; t++){
                        up_sum += epsilon[n][t][i][j];
                    }
                }
                model.transition[i][j] = up_sum / down_sum;
            }
        }
        //observation
        for (int i=0; i<model.state_num; i++){
            for (int k=0; k<model.observ_num; k++){
                double up_sum = 0.0;
                double down_sum = 0.0;
                for (int n=0; n<10000; n++){
                    for (int t=0 ;t<T; t++){
                        if ((seq[n][t]-'A')==k){
                            up_sum += gamma[n][t][i];
                        }
                        down_sum += gamma[n][t][i]; 
                    }
                }
                model.observation[k][i] = up_sum/down_sum;
            }
        }
    iter--;
    }
    FILE *fp2 = open_or_die(argv[4], "w");//output_model_path argv[4]
    dumpHMM(fp2, &model);
    //dumpHMM(stderr, &model);
    fclose(fp);
    fclose(fp2);

}