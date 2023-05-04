#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include "huffmanTree.cpp"
using namespace std;

Node *HuffmanCode(PriorityQueue &myQueue) { // Creates the Huffman Code Tree.
    bool complete = false;
    while (myQueue.getFront()->next->next != nullptr) {
        Node *temp = new Node();
        temp->sum = true;
        temp->tree = true;
        temp->left = myQueue.getFront();
        temp->right = myQueue.getFront()->next;
        temp->frequency = temp->left->frequency + temp->right->frequency;
        myQueue.popFront();
        myQueue.popFront();
        Node *cur = myQueue.getFront();
        Node *prev = new Node();
        if (myQueue.getFront()->frequency >= temp->frequency) {
            myQueue.makeFront(temp);
            temp->next = cur;
        }
        else {
            while (cur->frequency < temp->frequency) {
                prev = cur;
                cur = cur->next;
                if (cur == nullptr) {
                    break;
                }
            }
            prev->next = temp;
            temp->next = cur;
        }
    }
    Node *root = new Node();
    root->sum = true;
    root->tree = true;
    root->left = myQueue.getFront();
    root->right = myQueue.getFront()->next;
    root->frequency = root->left->frequency + root->right->frequency;
    myQueue.popFront();
    myQueue.popFront();
    return root;
}

void EncodeEdges(Node *node) { // Adds the binary coding to the nodes within the Huffman tree.
    if (node == nullptr) {
        return;
    }
    if (node->left != nullptr) {
        node->left->encoding += node->encoding;
        node->left->encoding += "0";
    }
    if (node->right != nullptr) {
        node->right->encoding += node->encoding;
        node->right->encoding += "1";
    }
    EncodeEdges(node->left);
    EncodeEdges(node->right);
}

void TreeTraversal(Node *node, vector<Node> &myCodes) { // Traverses the tree while updating the values of corresponding nodes.
    if (node == nullptr) {
        return;
    }
    TreeTraversal(node->left, myCodes);
    if (!node->sum) {
        //cout << "Symbol: " << node->symbol << ", Frequency: " << node->frequency << ", Code: " << node->encoding << endl;
        Node temp;
        temp.encoding = node->encoding;
        temp.symbol = node->symbol;
        temp.frequency = node->frequency;
        myCodes.push_back(temp);
    }
    TreeTraversal(node->right, myCodes);
}

void TraverseTreeAndPrint(Node *node, vector<Node> myCodes, string encoding, char &symbol) { // Recursively traverses the Huffman tree to find the matching symbol with the encoding.
    if (node == nullptr) {
        return;
    }
    TraverseTreeAndPrint(node->left, myCodes, encoding, symbol);
    if (node->encoding == encoding && !node->sum) {
        cout << "Symbol: " << node->symbol << ", Frequency: " << node->frequency << ", Code: " << node->encoding << endl; // Prints the line of corresponding information.
        symbol = node->symbol; // Updates the the symbol in memory to the thread.
        return;
    }
    TraverseTreeAndPrint(node->right, myCodes, encoding, symbol);
}

struct lines {
    string encoding; // Holds the encoding taken from the compressed portion of the input file.
    vector<int> positions; // Holds the positions taken from the corresponding encoding.
};

struct arguments {
    vector<Node> myCodes; // Holds the Huffman Tree.
    vector<lines> myCompressed; // Holds the compressed symbols with their positions in the output.
    Node *treeRoot; // Root of the Huffman Tree.
    int location = 0; // Keeps track of the location within the thread.
    int *turn; // Keeps track of the turn within the thread.
    pthread_mutex_t *bsem; // 
    pthread_cond_t *waitTurn;
    int length; // The length of the output.
    char *output; // Holds in memory the final output.
};

void *threadFunction(void *void_ptr) { 
    arguments *arg_ptr = (struct arguments *)void_ptr;
    char symbol = ' '; // Placeholder for the current symbol in the thread.
    int location = arg_ptr->location;
    arg_ptr->location = arg_ptr->location + 1;
    pthread_mutex_unlock(arg_ptr->bsem);
    pthread_mutex_lock(arg_ptr->bsem);
    while (*(arg_ptr->turn) != location) { // Continues to loop and wait until it is the symbol's turn.
        pthread_cond_wait(arg_ptr->waitTurn, arg_ptr->bsem);
    }
    pthread_mutex_unlock(arg_ptr->bsem); // If it is the symbol's turn, begin to process the needed steps.
    TraverseTreeAndPrint(arg_ptr->treeRoot, arg_ptr->myCodes, arg_ptr->myCompressed[*arg_ptr->turn].encoding, symbol); // Prints the symbol's information in a line.
    for (int i = 0; i < arg_ptr->myCompressed[*arg_ptr->turn].positions.size(); i++) { // Decompresses the symbol and finds the symbol's positions to place within the final output.
        arg_ptr->output[arg_ptr->myCompressed[*arg_ptr->turn].positions[i]] = symbol; // Updates positions within the output with the correct symbol.
    }
    pthread_mutex_lock(arg_ptr->bsem);
    *(arg_ptr->turn) = *(arg_ptr->turn) + 1;
    pthread_cond_broadcast(arg_ptr->waitTurn);
    pthread_mutex_unlock(arg_ptr->bsem);
    return NULL;
}

int main() {
    vector<Node> myList;
    vector<Node> myCodes;
    string input = "";
    cin >> input; // Read the input from STDIN. The first line has a single integer value representing the number of symbols in the alphabet (n).
    cin.ignore();
    const int n = stoi(input);
    for (int i = 0; i < n; i++) { // The next n lines present the information about the alphabet. Each line presents the information about a symbol from the alphabet:
        getline(cin, input);
        string temp = "";
        Node myNode;
        myNode.symbol = input[0];
        temp += input[2];
        myNode.frequency = stoi(temp);
        myList.push_back(myNode);
    }
    for (int i = 0; i < myList.size(); i++) { // Arranges symbols based on frequencies and/or ASCII value.
        for (int j = 0; j < myList.size() - 1; j++) {
            if (myList[j].frequency < myList[j+1].frequency) {
                Node temp = myList[j+1];
                myList[j+1] = myList[j];
                myList[j] = temp;
            }
            else if (myList[j].frequency == myList[j+1].frequency) {
                if (int(myList[j].symbol) > int(myList[j+1].symbol)) {
                    continue;
                }
                else if (int(myList[j].symbol) < int(myList[j+1].symbol)) {
                    Node temp = myList[j+1];
                    myList[j+1] = myList[j];
                    myList[j] = temp;                
                }
            }
        }
    }
    PriorityQueue myQueue = PriorityQueue();
    myQueue.queueList(myList);
    Node *treeRoot = HuffmanCode(myQueue); // Generate the Huffman Tree based on the input.
    EncodeEdges(treeRoot);
    TreeTraversal(treeRoot, myCodes);
    static pthread_mutex_t bsem;
    static pthread_cond_t waitTurn;
    pthread_mutex_init(&bsem, NULL);
    pthread_cond_init(&waitTurn, NULL);
    int turn = 0;
    arguments data;
    data.bsem = &bsem;
    data.waitTurn = &waitTurn;
    data.turn = &turn;
    data.myCodes = myCodes;
    data.treeRoot = treeRoot;
    int length = 0;
    for (int i = 0; i < n; i++) { // The final n lines present the information about the compressed file. Each line presents the information about a compressed symbol:
        getline(cin, input); 
        lines line;
        string code = "";
        int stop;
        for (int i = 0; i < input.length(); i++) {
            if (input[i] == ' ') {
                stop = i;
                break;
            }
            code += input[i];
        }
        line.encoding = code;
        string position = "";
        for (int i = stop + 1; i < input.length() + 1; i++) {
            if (input[i] == ' ' || i == input.length()) {
                line.positions.push_back(stoi(position));
                if (stoi(position) > length) { // Updates the length variable
                    length = stoi(position) + 1;
                }
                position = "";
                continue;
            }
            position += input[i];
        }
        data.myCompressed.push_back(line);
    }
    char *output = new char[length]; // A placeholder for the output in memory so that it may be manipulated by the threads.
    data.output = output;
    data.length = length;
    const int threads = n;
    pthread_t* pthread = new pthread_t[n];
    for (int i = 0; i < n; i++) { // Create n POSIX threads (where n is the number of symbols in the alphabet). Each child thread executes the following tasks:
        pthread_mutex_lock(&bsem);
        if (pthread_create(&pthread[i], nullptr, threadFunction, &data)) {
            fprintf(stderr, "Error creating thread\n");
            return 1;
        }
    }
    for (int i = 0; i < n; i++) { // Joins all the threads together.
            pthread_join(pthread[i], nullptr);
    }

    cout << "Original message: " << output; // Prints out the final decoded output.
    
    delete[] pthread;
    return 0;
}