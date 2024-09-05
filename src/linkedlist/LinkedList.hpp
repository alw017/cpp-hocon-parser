#include <hocon-p.hpp>

class LinkedList{
    // Struct inside the class LinkedList
    // This is one node which is not needed by the caller. It is just
    // for internal work.
    struct Node {
        int x;
        Node *next;
    };

// public member
public:
    // constructor
    LinkedList();

    // destructor
    ~LinkedList();
    
    // This prepends a new value at the beginning of the list
    void addValue(int val);

    // returns the first element in the list and deletes the Node.
    // caution, no error-checking here!
    int popValue();

// private member
private:
    Node *head; // this is the private member variable. It is just a pointer to the first Node
};