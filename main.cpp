#include <cmath>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#define HIDDENLAYERSIZE 10
#define pi 3.14
double weight1[10][3] = {
    {0.47033493164029083, 1.4098429917984185, -0.45868250608443023},
    {9.6050793142547, -4.8106381377184553, -6.2203249699239107},
    {31.770859194938183, -5.7448922199483343, -16.587627795598152},
    {-0.84940531846430345, 0.992233239423821, -1.0016216732907082},
    {-31.627591279841969, 6.372163860709442, 16.370799951110723},
    {-54.424139618749813, 18.259089064353208, 28.641186983131369},
    {-0.79810901615687035, 1.4504845684255849, -2.1005592298494506},
    {-0.4491767869239795, 0.029756397265863002, -0.13040976475951177},
    {-0.0095618922693373482, -0.033137398429464504, 0.00097940644650935786},
    {23.972375584069319, 1.39315413638509, -0.74824486890357866}};
double bias1[10] = {-64.662792988884348, 55.166473202056522,
                    -164.65949834408667, 15.211160979463422,
                    12.24130356452798,   17.021888846682156,
                    -27.158230855968867, 4.6294220239768293,
                    2.599812799324801,   -140.22968989032836};

double weight2[10] = {-2.0210328911392796,  1.4925632391411969,
                      0.44033520941192872,  -1.1474388699560774,
                      -0.84731807564734474, -0.44500692998149916,
                      -0.54580535770972349, 1.2629238917177057,
                      29.750097611943637,   -11.125476345495747};
double bias2 = -52.815634642162856;

bool inputLayerFinished = false;
bool hiddenLayerFinished = false;
bool outputLayerFinished = false;

sem_t *begin_input_layer;
sem_t *begin_calculate;
sem_t *begin_output_layer;
sem_t *begin_calculate_from_output_layer;
sem_t *begin_variance_calc;
sem_t *begin_output_from_var;

double x, y, z;
double x_temp, y_temp, z_temp;
double x_temp2, y_temp2, z_temp2;
double result;

double hiddenOutput[HIDDENLAYERSIZE];

double func(double in1, double in2, double in3) {
  return (in2 +
          sqrt(abs(in2 * in2 - 4 * in1 * in3)) / (2 * in1 + sin(in1 * pi)));
}

vector<string> split_by_space(string str) {
  istringstream buf(str);
  istream_iterator<string> beg(buf), end;
  vector<string> tokens(beg, end);
  return tokens;
}
void *inputLayer(void *) {
  ifstream myfile("./documentation/InputFile.txt");
  while (true) {
    // read from file
    double temp_input[3];
    if (myfile.is_open()) {
      if (!myfile.good()) {
        inputLayerFinished = true;
        sem_post(begin_calculate);
        return NULL;
      }
      string line;
      getline(myfile, line);
      vector<string> words = split_by_space(line);
      for (int i = 0; i < 3; i++) {
        temp_input[i] = stold(words[i].c_str());
      }
    }
    // put the input in array
    x = temp_input[0];
    y = temp_input[1];
    z = temp_input[2];
    // signal the lock
    sem_post(begin_calculate);
    sem_wait(begin_input_layer);
  }
}
void *hiddenLayer(void *params) {
  while (true) {
    sem_wait(begin_calculate);
    pthread_t threads[HIDDENLAYERSIZE];
    pthread_attr_t attr;
    x_temp = x;
    y_temp = y;
    z_temp = z;
    if (inputLayerFinished) {
      hiddenLayerFinished = true;
      sem_post(begin_output_layer);
      return NULL;
    }
    for (int i = 0; i < HIDDENLAYERSIZE; i++) {
      pthread_attr_init(&attr);
      pthread_create(&threads[i], &attr, neuron, (void *)i);
    }
    for (int i = 0; i < HIDDENLAYERSIZE; i++) {
      pthread_join(threads[i], NULL);
    }
    sem_post(begin_input_layer);
    sem_post(begin_output_layer);
    sem_wait(begin_calculate_from_output_layer);
  }
}
void *neuron(void *params) {
  int index = *((int *)&params);
  hiddenOutput[index] = tanh(x * weight1[index][0] + y * weight1[index][1] +
                             z * weight1[index][2] + bias1[index]);
}
void *outputLayer(void *params) {
  while (true) {
    if (hiddenLayerFinished) {
      sem_post(begin_variance_calc);
      outputLayerFinished = true;
      return NULL;
    }
    sem_wait(begin_output_layer);
    x_temp2 = x_temp;
    y_temp2 = y_temp;
    z_temp2 = z_temp;
    ofstream outfile;
    outfile.open("output.txt", std::ios_base::app);
    result = 0;
    for (int i = 0; i < HIDDENLAYERSIZE; i++) {
      result += hiddenOutput[i] * weight2[i];
    }
    result += bias2;
    outfile << result << endl;
    outfile.close();
    sem_post(begin_calculate_from_output_layer);
    sem_post(begin_variance_calc);
    if (hiddenLayerFinished) {
      sem_post(begin_variance_calc);
      outputLayerFinished = true;
      return NULL;
    }
    sem_wait(begin_output_from_var);
  }
}
void *variance_calculator(void *params) {
  int count = 0;
  double variance = 0;
  while (true) {
    sem_wait(begin_variance_calc);
    if (outputLayerFinished) {
      cout << sqrt(variance / count) << endl;
      return NULL;
    }
    double func_res = pow((func(x_temp2, y_temp2, z_temp2) - result), 2);
    if (isnan(variance))
      variance = func_res;
    else
      variance = func_res + variance;
    ++count;
    sem_post(begin_output_from_var);
  }
}

int main() {
  sem_unlink("/begin_input");
  begin_input_layer = sem_open("/begin_input", O_CREAT | O_EXCL, S_IRWXU, 0);
  if (begin_input_layer == SEM_FAILED) {
    perror("open begin_input");
    return 1;
  }

  sem_unlink("/begin_calculate");
  begin_calculate = sem_open("/begin_calculate", O_CREAT | O_EXCL, S_IRWXU, 0);
  if (begin_calculate == SEM_FAILED) {
    perror("open begin_calculate");
    return 1;
  }

  sem_unlink("/begin_output_layer");
  begin_output_layer =
      sem_open("/begin_output_layer", O_CREAT | O_EXCL, S_IRWXU, 0);
  if (begin_output_layer == SEM_FAILED) {
    perror("open begin_output_layer");
    return 1;
  }

  sem_unlink("/begin_calc_from_out_lay");
  begin_calculate_from_output_layer =
      sem_open("/begin_calc_from_out_lay", O_CREAT | O_EXCL, S_IRWXU, 0);
  if (begin_calculate_from_output_layer == SEM_FAILED) {
    perror("open begin_calc_from_out_lay");
    return 1;
  }
  sem_unlink("/begin_variance_calc");
  begin_variance_calc =
      sem_open("/begin_variance_calc", O_CREAT | O_EXCL, S_IRWXU, 0);
  if (begin_variance_calc == SEM_FAILED) {
    perror("open begin_variance_calc");
    return 1;
  }

  sem_unlink("/begin_output_from_var");
  begin_output_from_var =
      sem_open("/begin_output_from_var", O_CREAT | O_EXCL, S_IRWXU, 0);
  if (begin_output_from_var == SEM_FAILED) {
    perror("open begin_output_from_var");
    return 1;
  }

  pthread_t t1, t2, t3, t4;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&t1, &attr, inputLayer, NULL);
  pthread_create(&t2, &attr, hiddenLayer, NULL);
  pthread_create(&t3, &attr, outputLayer, NULL);
  pthread_create(&t4, &attr, variance_calculator, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);
  pthread_join(t4, NULL);

  return 0;
}