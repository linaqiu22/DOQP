// #include "client.hpp"
// #include "utility.hpp"
// #include <fstream>
// #include <list>
// #include <math.h>
// using namespace std;

// Client::Client(Predicate predicate) : predicate(predicate) {
//     T1 = make_shared<Table>(0);
//     T2 = make_shared<Table>(1);
//     output = new char[MAX_BUF_SIZE];
//     output_iter = output;
//     // if (predicate.attrID == 0) {
//     //     pointSanitizer = new unordered_map<number, Sanitizer>[2];
//     // } else {
//     //     pointSanitizer = new unordered_map<number, Sanitizer>[1];
//     // }
// }

// number Client::loadTables(string inputPath) {
//     ifstream in_file;
//     in_file.open(inputPath);
//     if (!in_file.is_open()) {
//         cout << "Error opening input file" << endl;
//         exit(0);
//     }
//     int n1, n2;
//     in_file >> n1 >> n2;
//     int n = n1 + n2;
//     number join_attr;
//     number data_attr;
//     for (int i = 0; i < n1; i++) {
//         in_file >> join_attr >> data_attr;
//         T1->data.push_back({0, join_attr, data_attr});
//     }
//     for (int i = n1; i < n; i++) {
//         in_file >> join_attr >> data_attr;
//         T2->data.push_back({1, join_attr, data_attr});
//     }
//     in_file.close();
//     sort(T1->data.begin(), T1->data.end());
//     sort(T2->data.begin(), T2->data.end());
//     T1->scan();
//     T2->scan();
//     number joinSpace = T1->tableSize * T2->tableSize;
//     return joinSpace;
// }

// void Client::preprocess() {
//     // One scan of data to construct list L
//     // Here completeKeyList is L in solution.pdf
//     auto it1 = T1->dist.begin(), it2 = T2->dist.begin();
//     while (it1 != T1->dist.end() && it2 != T2->dist.end()) {
//         if (it1->first < it2->first) {
//             completeKeyList.push_back({it1->second,0,it1->first,it1->second, 0});
//             it1++;
//         } else if (it1->first > it2->first) {
//             completeKeyList.push_back({0,it2->second,it2->first,0,it2->second});
//             it2++;
//         } else {
//             completeKeyList.push_back({it1->second,it2->second,it1->first,it1->second,it2->second});
//             it1++; it2++;
//         }
//     }
//     while (it1 != T1->dist.end()) {
//         completeKeyList.push_back({it1->second,0,it1->first,it1->second,0});
//         it1++;
//     }
//     while (it2 != T2->dist.end()) {
//         completeKeyList.push_back({0,it2->second,it2->first,0,it2->second});
//         it2++;
//     }
//     keyMax = completeKeyList.back().key;
//     //TODO implement trucated and shifted geometric distribution
//     int nAddedKeys = Table::truncatedLaplace(k_eps, k_delta, 1, 200, 0);
//     for (int i = 0; i < nAddedKeys; i++) {
//         completeKeyList.push_back({0,0,keyMax+i+1,0,0});
//     }

//     populatePointNoise(completeKeyList, nAddedKeys);
//     sort(completeKeyList.begin(), completeKeyList.end(),
//          [](const Entry& fst, const Entry& snd) {
//                 if (fst.n1 == snd.n1) {
//                     return fst.n2 > snd.n2;
//                 } else {
//                     return fst.n1 > snd.n1;
//                 }
//          });
//     // Build hierarchical sanitizer
//     populateRangeNoise(completeKeyList.size()*2);
//     T1->data.insert(T1->data.end(), T2->data.begin(), T2->data.end());
//     T1->tableSize += T2->tableSize;
//     //* outsource rangeSanitizer, completeKeyList, T1->data
// }

// // Add noise to list L 
// void Client::populatePointNoise(vector<Entry> &completeKeyList, int nAddedKeys) {
//     int buckets = 2*completeKeyList.size();
//     int level = ceil(log(buckets) / log(K)) + 1;
//     for (int l = 0; l < (int)completeKeyList.size(); l++) {
//         auto entry = completeKeyList[l];
//         for (int t = 0; t <= 1; t++) {
//             number noise = Table::truncatedLaplace(r_eps, r_delta, level, 200*level, 0);
//             // Sanitizer sanitizer;
//             if (t == 0) {
//                 // sanitizer = {entry.r1, noise};
//                 completeKeyList[l].n1 += noise;
//             } else { 
//                 // sanitizer = {entry.r2, noise};
//                 completeKeyList[l].n2 += noise;
//             }
//             // pointSanitizer[t][entry.key] = sanitizer;
//         }
//     }
//     /* Sanitizer dummy = {0, 0};
//     for (int i = 0; i < nAddedKeys; i++) {
//         pointSanitizer[0][keyMax+i+1] = dummy;
//         pointSanitizer[1][keyMax+i+1] = dummy;
//     } */
// }

// // Build hierarchical satinizer with constrained inferences, branch factor is K
// void Client::populateRangeNoise(int size) {
//     int buckets = size;
//     int level = ceil(log(buckets) / log(K)) + 1;
//     int remainder = 0;
//     // bottom up
//     for (int l = 0; l < level; l++) {
//         rangeSanitizer.push_back(vector<Sanitizer>());
//         for (int j = 0; j < buckets; j++) {
//             number noise = Table::truncatedLaplace(r_eps, r_delta, level, 200*level, 0);
//             Sanitizer sanitizer;
//             if (l == 0) {
//                 int keyPos = j / 2;
//                 int offset = j % 2;
//                 if (offset == 0) {
//                     sanitizer = {completeKeyList[keyPos].r1, completeKeyList[keyPos].n1-completeKeyList[keyPos].r1};
//                 } else {
//                     sanitizer = {completeKeyList[keyPos].r2, completeKeyList[keyPos].n2-completeKeyList[keyPos].r2};
//                 }
//             } else {
//                 number realSum = 0;
//                 number weightedNosiySum = 0;
//                 number endIdx = j == buckets-1 ? rangeSanitizer[l-1].size() : (j+1)*K;
//                 for (number k = j*K; k < endIdx; k++) {
//                     realSum += rangeSanitizer[l-1][k].realData;
//                     weightedNosiySum += rangeSanitizer[l-1][k].noise;
//                 }
//                 number z = (pow(K, l+1)-pow(K, l))/(pow(K, l+1)-1)*noise + (pow(K, l)-1)/(pow(K, l+1)-1)*weightedNosiySum;
//                 sanitizer = {realSum, z};
//             }
//             rangeSanitizer[l].push_back(sanitizer);
//         }
//         if (buckets % K > 0) {
//             remainder = 1;
//         } else {
//             remainder = 0;
//         }
//         buckets = buckets / K + remainder;
//     }
//     // top down
//     for (int l = level-1; l >= 1; l--) {
//         for (int j = 0; j < (int)rangeSanitizer[l].size(); j++) {
//             number childZSum = 0;
//             int leftChild = j*K;
//             int rightChild = min((int)rangeSanitizer[l-1].size(), (j+1)*K);
//             for (int k = leftChild; k < rightChild; k++) {
//                 childZSum += rangeSanitizer[l-1][k].noise;
//             }
//             float diff = (float)rangeSanitizer[l][j].noise - (float)childZSum;
//             diff /= (float)K;
//             for (int k = leftChild; k < rightChild; k++) {
//                 if (diff < 0 && abs(diff) > rangeSanitizer[l-1][k].noise) {
//                     rangeSanitizer[l-1][k].noise = 0;
//                 } else {
//                     rangeSanitizer[l-1][k].noise += diff;
//                 }
//             }
//         }
//     }
// }

// number Client::getInput(char *inputData) {
//     char *iter = inputData;
//     number inputSize = 0;
//     for (number i = 0; i < T1->tableSize; i++) {
//         memcpy(iter, &get<0>(T1->data[i]), sizeof(bool));
//         iter += sizeof(bool); inputSize += sizeof(bool);
//         memcpy(iter, &get<1>(T1->data[i]), sizeof(number));
//         iter += sizeof(number); inputSize +=  sizeof(number);
//         memcpy(iter, &get<2>(T1->data[i]), sizeof(number));
//         iter += sizeof(number); inputSize += sizeof(number);
//     }
//     return inputSize;
// }

// number Client::getKeyList(char *keyList) {
//     char *iter = keyList;
//     number keyListSize = 0;
//     for (auto e : completeKeyList) {
//         memcpy(iter, &e.key, sizeof(number));
//         iter += sizeof(number); keyListSize += sizeof(number);
//         memcpy(iter, &e.r1, sizeof(number));
//         iter += sizeof(number); keyListSize += sizeof(number);
//         memcpy(iter, &e.r2, sizeof(number));
//         iter += sizeof(number); keyListSize += sizeof(number);
//     }
//     return keyListSize;
// }

// void Client::getDPIdx(char *dpIdx, number &dpIdxSize, number &pSanitizerSize1, number &pSanitizerSize2) {
//     char *iter = dpIdx;
//     number unit_size = sizeof(number);
//     /* for (auto &&pr : pointSanitizer[0]) {
//         memcpy(iter, &pr.first, unit_size); iter += unit_size;
//         memcpy(iter, &pr.second.realData, unit_size); iter += unit_size;
//         memcpy(iter, &pr.second.noise, unit_size); iter += unit_size;
//     } */
//     pSanitizerSize1 = (iter - dpIdx);
//     /* for (auto &&pr : pointSanitizer[1]) {
//         memcpy(iter, &pr.first, unit_size); iter += unit_size;
//         memcpy(iter, &pr.second.realData, unit_size); iter += unit_size;
//         memcpy(iter, &pr.second.noise, unit_size); iter += unit_size;
//     } */
//     pSanitizerSize2 = iter - dpIdx - pSanitizerSize1;

//     for (number i = 0; i < rangeSanitizer.size(); i++) {
//         for (auto &&bucket : rangeSanitizer[i]) {
//             memcpy(iter, &i, unit_size); iter += unit_size;
//             memcpy(iter, &bucket.realData, unit_size); iter += unit_size;
//             memcpy(iter, &bucket.noise, unit_size); iter += unit_size;
//         }
//     }
//     dpIdxSize = (iter - dpIdx);
// }






