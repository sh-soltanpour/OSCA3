#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;
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
vector<string> split_by_space(string str) {
  istringstream buf(str);
  istream_iterator<string> beg(buf), end;
  vector<string> tokens(beg, end);
  return tokens;
}
int main() {
  ifstream myfile("./documentation/InputFile.txt");
  if (myfile.is_open()) {
    while (myfile.good()) {
      string line;
      getline(myfile, line);
      vector<string> words = split_by_space(line);
      double temp_input[3];
      for (int i = 0; i < 3; i++) {
        temp_input[i] = stold(words[i].c_str());
      }
      double x, y, z;
      x = temp_input[0];
      y = temp_input[1];
      z = temp_input[2];
      double output[10];
      for (int index = 0; index < 10; index++) {
        output[index] = tanh(x * weight1[index][0] + y * weight1[index][1] +
                             z * weight1[index][2] + bias1[index]);
      }
      double result = bias2;
      for (int index = 0; index < 10; index++) {
        result += output[index] * weight2[index];
      }
      ofstream outfile;
      outfile.open("validate_output.txt", std::ios_base::app);
      outfile << result << endl;
      outfile.close();
    }
  }
}