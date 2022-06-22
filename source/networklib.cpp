/**
* Classes for generating a network, perform percolation and compute properties
* @file networklib.cpp
* @author Alessandro Canevaro
* @version 11/06/2022
*/

#include <iostream>
#include <random>
#include <fstream>
#include <vector>
#include <numeric>
#include <sstream>
#include <algorithm>
#include <../include/networklib.h>

using namespace std;

Network::Network(int n){
    nodes = n; // number of nodes
    vector<int> sequence;
    vector<vector<int>> network;
}

Network::Network(string path){
    vector<int> v1, v2;
    string line;
    ifstream myfile (path);

    if (myfile.is_open()){
        while (getline(myfile, line)){
            vector<string> params_value(2);
            string str1 = line.substr(0, line.find(','));
            string str2 = line.substr(line.find(',')+1, line.size());
            v1.push_back(stoi(str1));
            v2.push_back(stoi(str2));
        }
        myfile.close();
    }
    else{
        cout << "Unable to open file";
    } 

    int m1 = *max_element(v1.begin(), v1.end());
    int m2 = *max_element(v2.begin(), v2.end());

    nodes = (m1 > m2) ? m1 : m2;
    nodes++; //count node 0
    vector<int> sequence;
    vector<vector<int>> net(nodes);

    for(int i=0; i<v1.size(); i++){
        net[v1[i]].push_back(v2[i]);
        net[v2[i]].push_back(v1[i]);
    }
    network = net;
}

void Network::generateUniformDegreeSequence(int a, int b){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib(a, b);

    vector<int> degree_sequence (nodes);

    int sum = 1;
    while (sum % 2 != 0){ //generate a sequence until the total number of stubs is even
        for (int i=0; i<nodes; i++){
            degree_sequence[i] = distrib(gen);
        }
        sum = accumulate(degree_sequence.begin(), degree_sequence.end(), 0);
    }

    sequence = degree_sequence;
}

void Network::generateBinomialDegreeSequence(int n, float p){
    random_device rd;
    mt19937 gen(rd());
    binomial_distribution<> distrib(n, p);

    vector<int> degree_sequence (nodes);

    int sum = 1;
    while (sum % 2 != 0){ //generate a sequence until the total number of stubs is even
        for (int i=0; i<nodes; i++){
            degree_sequence[i] = distrib(gen);
        }
        sum = accumulate(degree_sequence.begin(), degree_sequence.end(), 0);
    }

    sequence = degree_sequence;
}

void Network::generatePowerLawDegreeSequence(float alpha){
    random_device rd;
    mt19937 gen(rd());
    vector<double> intervals;
    vector<double> weights;
    //double c = 0;
    for(int i=1; i < (int) sqrt(nodes) +1; i++){
        intervals.push_back((double) i);
        weights.push_back(pow(i, -alpha));
        //c += pow(i, -alpha);
    }

    /*
    double mean = 0;
    for(int i=1; i < (int) sqrt(nodes) +1; i++){
        mean += i*pow(i, -alpha)/c;
    }
    cout << "theoretical mean: " << mean << endl;
    */
    piecewise_constant_distribution<double> distribution(intervals.begin(), intervals.end(), weights.begin());

    vector<int> degree_sequence (nodes);

    int sum = 1;
    while (sum % 2 != 0){ //generate a sequence until the total number of stubs is even
        for (int i=0; i<nodes; i++){
            degree_sequence[i] = (int) distribution(gen);
            //cout << degree_sequence[i] << ' ';
        }
        sum = accumulate(degree_sequence.begin(), degree_sequence.end(), 0);
    }
    
    sequence = degree_sequence;
}

float Network::getDegreeDistMean(){
    return reduce(sequence.begin(), sequence.end()) / (float)sequence.size();
}

void Network::matchStubs(){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distrib(0, nodes-1);

    vector<int> degree_sequence = sequence;
    vector<vector<int>> net (nodes);

    for(int i=0; i<nodes; i++){
        while(degree_sequence[i] > 0){
            //chose another (valid) node at random
            int node = -1;
            while(true){
                node = distrib(gen);
                if (node == i){ //self-edge case: we can stop if the node has at least 2 stubs
                    if (degree_sequence[node] > 1){
                        break;
                    }
                }
                else{ //we can stop if the node has at least 1 stub
                    if (degree_sequence[node] > 0){
                        break;
                    }
                }
            }

            //put the match in the two vectors of network
            net[i].push_back(node);
            net[node].push_back(i);

            //decrease the degree of the two nodes in the sequence
            degree_sequence[i]--;
            degree_sequence[node]--;
        }
    }
    //sequence = degree_sequence;
    network = net;

}

void Network::removeSelfMultiEdges(){
    vector<vector<int>> net(nodes);
    for(int i=0; i<network.size(); i++){
        vector<int> node = network[i];
        sort(node.begin(), node.end());
        node.push_back(-1); //extra value
        for(int j=0; j<network[i].size(); j++){
            if(node[j] == i){
                //self loop
                continue;
            }
            if(node[j] == node[j+1]){
                //self loop or multiedge
                continue;
            }
            net[i].push_back(node[j]);
        }
    }
    network = net;
}

void Network::printNetwork(){
    for(int i=0; i<nodes; i++){
        cout << "node " << i << " is connected to: ";
        for(auto j: network[i])  cout << j << ' ';
        cout << endl;
    }
}

void Network::generateUniformOrder(){
    random_device rd;
    mt19937 gen(rd());

    vector<int> order;
    for(int i=0; i<nodes; i++){
        order.push_back(i);
    }
    shuffle(order.begin(), order.end(), gen);

    node_order = order;
}

void Network::nodePercolation(){
    vector<int> labels(nodes);
    fill(labels.begin(), labels.end(), -1);

    vector<vector<int>> net(nodes); //contains nodes
    vector<int> cluster_size(nodes); //contains labels to size
    fill(cluster_size.begin(), cluster_size.end(), 0);
    int new_label = 0;

    vector<int> result;
    int max_size = 1;

    for(int n: node_order){
        //cout << "node: " << n << endl;

        labels[n] = new_label;
        cluster_size[new_label]++;
        new_label++;
        for(int n2: network[n]){
            if(labels[n2] != -1){
                //cout << "adding edge between node " << n << " and " << n2 << endl;
                net[n].push_back(n2);
                net[n2].push_back(n);

                if(labels[n] != labels[n2]){ //merge
                    //cout << "merging cluster " << labels[n] << " with cluster " << labels[n2] << endl; 
                    vector<int> frontier; //contains nodes
                    int lab;
                    if(cluster_size[labels[n]]>cluster_size[labels[n2]]){
                        frontier.push_back(n2);
                        lab = labels[n];                
                    }
                    else{
                        frontier.push_back(n);
                        lab = labels[n2];     
                    }

                    cluster_size[lab] += cluster_size[labels[frontier.back()]];
                    cluster_size[labels[frontier.back()]] = 0;
                    if(cluster_size[lab] > max_size){
                        max_size = cluster_size[lab];
                    }

                    //cout << "lab: " << lab << endl;
                    while (frontier.size() > 0){
                        /*cout << "nodes in frontier: ";
                        for (int f: frontier){
                            cout << f <<" ";
                        }
                        cout << endl;*/

                        if(labels[frontier.back()] != lab){
                            //cout << "changing stuff" << endl;
                            labels[frontier.back()] = lab;
                            frontier.insert(frontier.begin(), net[frontier.back()].begin(), net[frontier.back()].end());
                        }
                        frontier.pop_back();
                    }
                }
            }
        }
        //cout << "max size: " << max_size << endl;
        result.push_back(max_size);
        /*
        cout << "labels: ";
        for(int a: labels){
            cout << a << ", ";
        }
        cout << endl;
        cout << "size: ";
        for(int b: cluster_size){
            cout << b << ", ";
        }
        cout << endl;
        */
    }
    sr = result;
}

void Network::rewire(float p){
    //while no edges left
    //choose two edges and remove them from the current net
    //swap them and add them to the new network

    //get edge list
    //shuffle edge list
    //for edge in edge list
    //  take edge i and i-1, swap them and add them to the new net only if the new edges doesn already exists
    // otherwise try another combination or do something else idk

    vector<vector<int>> edgelist;
    for(int n=0; n<nodes; n++){
        for(int n2: network[n]){
            if(n2 > n){
                vector<int> edge{n, n2};
                edgelist.push_back(edge);
            }
        }
    }

    random_device rd;
    mt19937 gen(rd());

    shuffle(edgelist.begin(), edgelist.end(), gen);

    vector<vector<int>> net(nodes);
    cout << "edgelist size " << edgelist.size() << endl;
    while(edgelist.size() > 0){
        int n1 = edgelist[edgelist.size()-1][0];
        int n2 = edgelist[edgelist.size()-1][1];
        int n3 = edgelist[edgelist.size()-2][0];
        int n4 = edgelist[edgelist.size()-2][1]; 

        //check that there are 4 distinct nodes (we already know n1 != n2 and n3 != n4)
        bool cond = n1 != n3 && 
                    n2 != n4 && 
                    n1 != n4 && 
                    n2 != n3;
        
        bool cond1 = cond &&
                     find(net[n1].begin(), net[n1].end(), n3) == net[n1].end() && 
                     find(net[n2].begin(), net[n2].end(), n4) == net[n2].end();
        bool cond2 = cond &&
                     find(net[n1].begin(), net[n1].end(), n4) == net[n1].end() &&
                     find(net[n2].begin(), net[n2].end(), n3) == net[n2].end();
        cout << n1 << ", " << n2 << ", " << n3 << ", " << n4 << endl;

        if(cond1){
            cout << "in cond 1, adding edge between: " << n1 << "-" << n3 << " and " << n2 << "-" << n4 << endl;
            //switch
            net[n1].push_back(n3);
            net[n3].push_back(n1);
            net[n2].push_back(n4);
            net[n4].push_back(n2);
            //pop both
            edgelist.pop_back();
            edgelist.pop_back();
        }
        else if(cond2){
            cout << "in cond 2, adding edge between: " << n1 << "-" << n4 << " and " << n2 << "-" << n3 << endl;
            //switch
            net[n1].push_back(n4);
            net[n4].push_back(n1);
            net[n2].push_back(n3);
            net[n3].push_back(n2);
            //pop both
            edgelist.pop_back();
            edgelist.pop_back();

        }
        else{
            cout << "in else" << endl;
            //put first edge in the front
            vector<int> new_edge{n1, n2};
            edgelist.insert(edgelist.begin(), new_edge);
            //pop last
            edgelist.pop_back();
        }
    }

    network = net;
    cout << "rewire done" << endl;
}

vector<vector<int>> Network::getNetwork(){
    return network;
}

vector<int> Network::getSr(){
    return sr;
}


GiantCompSize::GiantCompSize(){
    vector<vector<int>> sr_mat;
}

void GiantCompSize::generateNetworks(int net_num, int net_size, char type, float param1, float param2){
    vector<vector<int>> sr_matrix; 
    for(int i=0; i<net_num; i++){
        Network net = Network(net_size);
        switch (type){
        case 'u':
            net.generateUniformDegreeSequence((int) param1, (int) param2);
            break;
        case 'b':
            net.generateBinomialDegreeSequence((int) param1, param2);
            break;
        case 'p':
            net.generatePowerLawDegreeSequence(param1);
            break;
        }
        //cout << "average degree: " << net.getDegreeDistMean() << endl;
        net.matchStubs();
        net.removeSelfMultiEdges();
        //cout << net.getDegreeDistMean() << endl;
        net.generateUniformOrder();
        net.nodePercolation();
        sr_matrix.push_back(net.getSr());
    }
    sr_mat = sr_matrix;
}

vector<double> GiantCompSize::computeAverageGiantClusterSize(int bins){
    vector<vector<int>> sr_mat_t = this->transpose(sr_mat);
    vector<double> avg_sr;
    for(int i=0; i<sr_mat_t.size(); i++){
        avg_sr.push_back(this->average(sr_mat_t[i]));
    }
    vector<double> result;

    for(int i=0; i<bins; i++){
        double tmp = 0;
        for(int r=0; r<avg_sr.size()+1; r++){
            tmp += bin_pmf[i][r]*avg_sr[r];
        }
        result.push_back(tmp);
    }
    return result;
}

double GiantCompSize::computeGiantClusterSize(float phi, vector<double> sr){
    vector<double> pmf = this->getBinomialPMF(phi, sr.size());
    double result = 0;
    for(int r=1; r<sr.size(); r++){
        result += pmf[r]*sr[r-1];
    }
    return result;
}

void GiantCompSize::loadBinomialPMF(string path){
    cout << "loading pmf: " << path << endl;
    vector<vector<double>> result;
    string line;
    ifstream myfile (path);

    if (myfile.is_open()){;
        while (getline(myfile, line)){
            //cout << "reading line: " << result.size() << endl;
            vector<double> row;
            while(true){
                auto pos = line.find(',');
                if(pos != string::npos){
                    string str = line.substr(0, pos);
                    line.erase(0, pos+1);
                    //cout << str << endl;
                    try{
                        row.push_back(stod(str));}
                        catch (out_of_range){
                            cout << "out of range " << str << endl;
                        }
                }
                else{
                    break;
                }
            }
            result.push_back(row);
        }
        myfile.close();
    }
    else{
        cout << "Unable to open file" << endl;
    }
    bin_pmf = result;
}

vector<double> GiantCompSize::getBinomialPMF(float phi, int nodes){
    vector<double> pmf(nodes + 1, 0.0);
    pmf[0] = 1.0;

    auto k = 0;
    for (auto n = 0; n < nodes; n++) {
        for (k = n + 1; k > 0; --k) {
        pmf[k] += phi * (pmf[k - 1] - pmf[k]);
        }
        pmf[0] *= (1 - phi);
    }
    return pmf;
}

vector<vector<int>> GiantCompSize::transpose(vector<vector<int>> data){
    vector<vector<int> > result(data[0].size(), vector<int>(data.size()));
    for (vector<int>::size_type i = 0; i < data[0].size(); i++) 
        for (vector<int>::size_type j = 0; j < data.size(); j++) {
            result[i][j] = data[j][i];
        }
    return result;
}

double GiantCompSize::average(vector<int> data){
    return reduce(data.begin(), data.end()) / (double)data.size();
}