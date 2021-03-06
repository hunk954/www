#include"fptree/fptree.h"
#include <algorithm>

using namespace std;
// Initial the new InnerNode
InnerNode::InnerNode(const int& d, FPTree* const& t, bool _isRoot) {
    // done
    this->degree = d;
    this->isLeaf = false;
    this->tree = t;

    this->isRoot = _isRoot;
    this->keys = new Key[2 * d + 1];
    this->childrens = new Node*[2*d+2];
    for(int i = 0; i < 2*d+2; i++){
        childrens[i] = NULL;
    }
    this->nKeys = 0;
    this->nChild = 0;
}

// delete the InnerNode
InnerNode::~InnerNode() {
    // done
    delete this->childrens;
    delete this->keys;
}

// binary search the first key in the innernode larger than input key
int InnerNode::findIndex(const Key& k) {
    // done
    //k0:c0&c1, k1:c2, k2:c3, ... kn:cn+1
    int low = 0, high = nKeys - 1, mid;
    while(low <= high){
        mid = (low + high) / 2;
        if (k == keys[mid]) 
            return mid + 1;
        else if (k < keys[mid]) 
            high = mid - 1;
        else 
            low = mid + 1;
        
    }
    return low;
}

// insert the node that is assumed not full
// insert format:
// ======================
// | key | node pointer |
// ======================
// WARNING: can not insert when it has no entry
void InnerNode::insertNonFull(const Key& k, Node* const& node) {
    // done
    int index = findIndex(k);
    int i;
    for (i = nKeys; i > index; i--)
        keys[i] = keys[i - 1];
    keys[index] = k;
    nKeys++;
    for (i = nChild; i > index + 1; i--)
        childrens[i] = childrens[i - 1];
    childrens[index + 1] = node;
    nChild++;
}

// insert func
// return value is not NULL if split, returning the new child and a key to insert
KeyNode* InnerNode::insert(const Key& k, const Value& v) {
    KeyNode* newChild = NULL;
    // 1.insertion to the first leaf(only one leaf)
    if (this->isRoot && this->nKeys == 0) {
        // done
        if (nChild == 0) {
            LeafNode *newLeaf = new LeafNode(tree);
            childrens[nChild++] = newLeaf;
        }
        newChild = childrens[0]->insert(k, v);
        if (newChild != NULL) {
            insertLeaf(*newChild);
        }
        return newChild;
    }
    
    // 2.recursive insertion
    // done
    int index = findIndex(k);
    newChild = childrens[index]->insert(k, v);
    if (newChild != NULL) {
        if (nKeys < 2 * degree) {
            insertNonFull(newChild->key, newChild->node);
            newChild = NULL;
        }   
        else {
            insertNonFull(newChild->key, newChild->node);
            newChild = split();
            if (this->isRoot) {
                this->isRoot = false;
                InnerNode *newRoot = new InnerNode(this->degree, this->tree, true);
                newRoot->childrens[newRoot->nChild++] = this;
                newRoot->insertNonFull(newChild->key, newChild->node);
                this->tree->changeRoot(newRoot);
            }
        } 
    }

    return newChild;
}

// ensure that the leaves inserted are ordered
// used by the bulkLoading func
// inserted data: | minKey of leaf | LeafNode* |
KeyNode* InnerNode::insertLeaf(const KeyNode& leaf) {
    KeyNode* newChild = NULL;
    // first and second leaf insertion into the tree
    if (this->isRoot && this->nKeys == 0) {
        // done
        if (nChild == 0) {
            childrens[nChild++] = leaf.node;
        } else {
            insertNonFull(leaf.key, leaf.node);
        }

        return newChild;
    }
    
    // recursive insert
    // Tip: please judge whether this InnerNode is full
    // next level is not leaf, just insertLeaf
    // done
    int index = findIndex(leaf.key);
    if (!childrens[index]->ifLeaf()) {
        newChild = dynamic_cast<InnerNode *>(childrens[index])->InnerNode::insertLeaf(leaf);
        if (newChild != NULL) {
            if (nKeys < 2 * degree) {
                insertNonFull(newChild->key, newChild->node);
                newChild = NULL;
            }   
            else {
                insertNonFull(newChild->key, newChild->node);
                newChild = split();
                if (this->isRoot) {
                    this->isRoot = false;
                    InnerNode *newRoot = new InnerNode(this->degree, this->tree, true);
                    newRoot->childrens[newRoot->nChild++] = this;
                    newRoot->insertNonFull(newChild->key, newChild->node);
                    this->tree->changeRoot(newRoot);
                }
            } 
        }
    } else {  // next level is leaf, insert to childrens array
        if (nChild < 2 * degree + 1) {
            insertNonFull(leaf.key, leaf.node);
            newChild = NULL;
        } else {
            insertNonFull(leaf.key, leaf.node);
            newChild = split();
            if (this->isRoot) {
                    this->isRoot = false;
                    InnerNode *newRoot = new InnerNode(this->degree, this->tree, true);
                    newRoot->childrens[newRoot->nChild++] = this;
                    newRoot->insertNonFull(newChild->key, newChild->node);
                    this->tree->changeRoot(newRoot);
                }
        }
    }

    return newChild;
}

KeyNode* InnerNode::split() {
    KeyNode* newChild = new KeyNode();
    // right half entries of old node to the new node, others to the old node. 
    // done
    InnerNode* newNode = new InnerNode(this->degree, this->tree, false);
    int i;
    newNode->childrens[0] = this->childrens[this->degree + 1];
    newNode->childrens[1] = this->childrens[this->degree + 2];
    newNode->keys[0] = this->keys[this->degree + 1];
    newNode->nChild = 2;
    newNode->nKeys = 1;
    for (i = this->degree + 2; i < 2 * this->degree + 1; i++) {
        newNode->insertNonFull(this->keys[i], this->childrens[i + 1]);
    }
    newChild->key = keys[this->degree];
    newChild->node = newNode;
    this->nKeys = this->degree;
    this->nChild = this->degree + 1;

    return newChild;
}

// remove the target entry
// return TRUE if the children node is deleted after removement.
// the InnerNode need to be redistributed or merged after deleting one of its children node.
bool InnerNode::remove(const Key& k, const int& index, InnerNode* const& parent, bool &ifDelete) {
    // only have one leaf
    // done
    if (this->nChild == 1 && this->isRoot) {
        if (childrens[0]->remove(k, 0 ,this, ifDelete) == false) return false;
        if(!ifDelete) return true;
        delete childrens[0];
        nChild--;
        return true;
    }
           
    // recursive remove
    // TODO:
    int nextindex = findIndex(k);
    if (childrens[nextindex]->remove(k, nextindex ,this, ifDelete) == false) return false;
    if(!ifDelete) return true;
    //从this中移出 childrens[i]
    // if (k == 220) cout << nextindex << endl;
    if (childrens[nextindex]->ifLeaf()) {
        removeChild(nextindex, nextindex);
    }  
    else
        removeChild(nextindex, nextindex + 1);

    //接着检查最小占有情况
    //通常情况
    if(this->isRoot || nKeys >= degree) {
        ifDelete = false;
        return true;
    }  

    //else 当前节点元素不够
    InnerNode * leftBro, * rightBro;
    getBrother(index, parent, leftBro, rightBro);
    //如果兄弟有多余项，重分布；否则，合并
    //与右兄弟重分布
    if(rightBro && rightBro->nKeys > degree) {
        ifDelete = false;
        redistributeRight(index, rightBro, parent);
    }     

    // 当前节点元素不够，与左兄弟重分布
    else if(leftBro && leftBro->nKeys > degree) {
        ifDelete = false;
        redistributeLeft(index, leftBro, parent);
    }
        
    // 当前节点元素不否，父亲只有两个孩子(左或右)且父亲节点为根节点，合并这三者
    else if(parent->nChild == 2 && parent->isRoot){
        if(rightBro) {
            mergeParentRight(parent, rightBro);
        }     
        else {
            mergeParentLeft(parent, leftBro);
        }   
    }    
    // 当前节点元素不够，与右兄弟合并
    else if(rightBro) {
        mergeRight(rightBro, parent->keys[index]);
        // if (k == 220) {
        //     printNode();
        //     cout << nKeys << endl;}
    }
        

    // 当前节点元素不够，与左兄弟合并
    else if(leftBro) {
        //key = ?
        mergeLeft(leftBro, parent->keys[index - 1]); 
    }
         

    return true;
}

// If the leftBro and rightBro exist, the rightBro is prior to be used
void InnerNode::getBrother(const int& index, InnerNode* const& parent, InnerNode* &leftBro, InnerNode* &rightBro) {
    // done
    if (index > 0)  //结点不是第一个孩子，有左兄弟
        leftBro = (InnerNode *)parent->childrens[index - 1];
    else
        leftBro = NULL;

    if (index < parent->nChild - 1) //结点不是最后一个孩子，有右兄弟
        rightBro = (InnerNode *)parent->childrens[index + 1];
    else
        rightBro = NULL;
}

// merge this node, its parent and left brother(parent is root)
void InnerNode::mergeParentLeft(InnerNode* const& parent, InnerNode* const& leftBro) {
    // TODO:
    leftBro->keys[degree - 1] = parent->keys[0];
    leftBro->nKeys++;
    for (int i = degree, j = 0; j < this->nChild; i++, j++) {
        leftBro->childrens[i] = this->childrens[j];
        leftBro->nChild++;
        if (j < this->nKeys) {
            leftBro->keys[i] = this->keys[j];
            leftBro->nKeys++;
        }
    }
    this->isRoot = true;
    this->tree->changeRoot(this);
    delete parent;
    // delete this;
}

// merge this node, its parent and right brother(parent is root)
void InnerNode::mergeParentRight(InnerNode* const& parent, InnerNode* const& rightBro) {
    // TODO:
    keys[degree - 1] = parent->keys[0];
    nKeys++;
    for (int i = degree, j = 0; j < rightBro->nChild; i++, j++) {
        childrens[i] = rightBro->childrens[j];
        nChild++;
        if (j < rightBro->nKeys) {
            keys[i] = rightBro->keys[j];
            nKeys++;
        }
    }
    this->isRoot = true;
    this->tree->changeRoot(this);
    delete parent;
    // delete rightBro;
}

// this node and its left brother redistribute
// the left has more entries
void InnerNode::redistributeLeft(const int& index, InnerNode* const& leftBro, InnerNode* const& parent) {
    // TODO:
    InnerNode *indexChild = (InnerNode *)parent->childrens[index];
    for(int i = indexChild->nKeys; i > 0 ; i--)
        indexChild->keys[i] = indexChild->keys[i-1];
    for(int i = indexChild->nChild; i > 0; i--)
        indexChild->childrens[i] = indexChild->childrens[i-1];
    
    indexChild->keys[0] = parent->keys[index-1];
    indexChild->childrens[0] = leftBro->childrens[nChild-1];
    parent->keys[index-1] = leftBro->keys[nKeys-1];

    indexChild->nKeys++;
    indexChild->nChild++;
    leftBro->nKeys--;
    leftBro->nChild--;
    //*************

    
}

// this node and its right brother redistribute
// the right has more entries
void InnerNode::redistributeRight(const int& index, InnerNode* const& rightBro, InnerNode* const& parent) {
    // TODO:
    /*InnerNode *indexChild = (InnerNode *)parent->childrens[index];
    indexChild->keys[nKeys] = rightBro->keys[0];
    for(int i = 0; i < rightBro->nKeys-1; i++)
        rightBro->keys[i] = rightBro->keys[i+1];
    indexChild->nKeys++;
    rightBro->nKeys--;
    parent->keys[index] = rightBro->keys[0];*/

    //***************
    this->childrens[nChild] = rightBro->childrens[0];
    this->nChild++;
    rightBro->nChild--;
    
    this->keys[nKeys] = parent->keys[index];
    parent->keys[index] = rightBro->keys[0];
    rightBro->nKeys--;
    this->nKeys++;
    for(int i = 0; i < nKeys; i++)
        rightBro->keys[i] = rightBro->keys[i + 1];
    for(int i = 0; i < nChild; i++)
        rightBro->childrens[i] = rightBro->childrens[i+1];
}

// merge all entries to its left bro, delete this node after merging.
void InnerNode::mergeLeft(InnerNode* const& leftBro, const Key& k) {
    // done
    leftBro->keys[degree - 1] = k;
    leftBro->nKeys++;
    for (int i = degree, j = 0; j < this->nChild; i++, j++) {
        leftBro->childrens[i] = this->childrens[j];
        leftBro->nChild++;
        if (j < this->nKeys) {
            leftBro->keys[i] = this->keys[j];
            leftBro->nKeys++;
        }
    }
}

// merge all entries to its right bro, delete this node after merging.
void InnerNode::mergeRight(InnerNode* const& rightBro, const Key& k) {
    // done
    keys[degree - 1] = k;
    nKeys++;
    for (int i = degree, j = 0; j < rightBro->nChild; i++, j++) {
        childrens[i] = rightBro->childrens[j];
        nChild++;
        if (j < rightBro->nKeys) {
            keys[i] = rightBro->keys[j];
            nKeys++;
        }
    }
}

// remove a children from the current node, used by remove func
void InnerNode::removeChild(const int& keyIdx, const int& childIdx) {
    // TODO:
    if (nKeys > 0 && keyIdx >= 0) {
        //如果是最后一个孩子，即此孩子没有右兄弟，如果需要remove只能remove左边的key
        for(int i = (keyIdx != nKeys ? keyIdx : keyIdx - 1); i < nKeys - 1; i++)
            keys[i] = keys[i + 1];
        nKeys--;
    }
    if (nChild > 0 && childIdx >= 0) {
        delete childrens[childIdx];
        for(int i = (childIdx != nChild ? childIdx : childIdx - 1); i < nChild - 1; i++)
            childrens[i] = childrens[i + 1];
        nChild--;
    }  
}

// update the target entry, return true if the update succeed.
bool InnerNode::update(const Key& k, const Value& v) {
    // done
    int index = findIndex(k);

    return childrens[index]->update(k, v);
}

// find the target value with the search key, return MAX_VALUE if it fails.
Value InnerNode::find(const Key& k) {
    // done
    int index = findIndex(k);
        
    return childrens[index]->find(k);
}

// get the children node of this InnerNode
Node* InnerNode::getChild(const int& idx) {
    // done
    if (idx < this->nChild) {
        return this->childrens[idx];
    }
    return NULL;
}

// get the key of this InnerNode
Key InnerNode::getKey(const int& idx) {
    if (idx < this->nKeys) {
        return this->keys[idx];
    } else {
        return MAX_KEY;
    }
}

// print the InnerNode
void InnerNode::printNode() {
    cout << "||#|";
    for (int i = 0; i < this->nKeys; i++) {
        cout << " " << this->keys[i] << " |#|";
    }
    cout << "|" << "    ";
}

// print the LeafNode
void LeafNode::printNode() {
    cout << "||";
    for (int i = 0; i < 2 * this->degree; i++) {
        if (this->getBit(i)) {
            cout << " " << this->kv[i].k << " : " << this->kv[i].v << " |";
        }
    }
    cout << "|" << " ====>> ";
}

// new a empty leaf and set the valuable of the LeafNode
LeafNode::LeafNode(FPTree* t) {
    // DONE
    tree = t;
    isLeaf = true;
    degree = LEAF_DEGREE;
    int nn = LEAF_DEGREE * 2;
    bitmapSize = (nn + 7) / 8;    
    
    PAllocator::getAllocator()->getLeaf(pPointer,pmem_addr);
    
    // the pointer below are all pmem address based on pmem_addr
    bitmap = (Byte*)pmem_addr;
    pNext = (PPointer*)(pmem_addr + bitmapSize);
    fingerprints = (Byte*)(pNext) + sizeof(PPointer);
    kv = (KeyValue*)(fingerprints + nn * sizeof(Byte));

    n = 0;
    prev = next = NULL;
    filePath = DATA_DIR + to_string(pPointer.fileId);
}

// reload the leaf with the specific Persistent Pointer
// need to call the PAllocator
LeafNode::LeafNode(PPointer p, FPTree* t) {
    // DONE
    tree = t;
    isLeaf = true;
    degree = LEAF_DEGREE;

    int nn = LEAF_DEGREE * 2;
    bitmapSize = (nn + 7) / 8;     
   
    pPointer = p;
    pmem_addr = PAllocator::getAllocator()->getLeafPmemAddr(p);
    
    // the pointer below are all pmem address based on pmem_addr
    bitmap = (Byte*)pmem_addr;
    pNext = (PPointer*)(pmem_addr + bitmapSize);
    fingerprints = (Byte*)(pNext) + sizeof(PPointer);
    kv = (KeyValue*)(fingerprints + nn * sizeof(Byte));
    
    n = 0;
    prev = next = NULL;
    filePath = DATA_DIR + to_string(pPointer.fileId);

    
    for(uint64_t i = 0; i < bitmapSize; i++)
        if(getBit(i)) n++;
}

LeafNode::~LeafNode() {
    // TODO:
    persist();
}

// insert an entry into the leaf, need to split it if it is full
KeyNode* LeafNode::insert(const Key& k, const Value& v) {
    KeyNode* newChild = NULL;
    // TODO     
    if(n == 2*degree - 1) {
        insertNonFull(k,v);
        newChild = split();
    }   
    else 
        insertNonFull(k,v);
    return newChild;
}

inline void setBit(Byte *bitmap,int slot){
    bitmap[slot / 8] |= 0x01<<(7-(slot % 8));
}

inline void clearBit(Byte *bitmap, int slot){
    bitmap[slot / 8] &= ~(0x01<<(7-(slot % 8)));
}

// insert into the leaf node that is assumed not full
void LeafNode::insertNonFull(const Key& k, const Value& v) {
    // DONE
    /*From Oukid_FPTree.pdf Algorithm2:
        slot = Leaf.Bitmap.FindFirstZero();
        Leaf.KV[slot] = (K, V); Leaf.Fingerprints[slot] = hash(K);
        //Persist(Leaf.KV[slot]); Persist(Leaf.Fingerprints[slot]);
        Leaf.Bitmap[slot] = 1; 
        //Persist(Leaf.Bitmap);
    */
    int slot = findFirstZero();
    kv[slot].k = k;
    kv[slot].v = v;
    fingerprints[slot] = keyHash(k);
    setBit(bitmap,slot);
    // bitmap[slot / 8] |= 0x01>>(7-(slot % 8));
    n++;
    // persist();
}

// split the leaf node
KeyNode* LeafNode::split() {
    KeyNode* newChild = new KeyNode();
    // DONE
    Key split_key = findSplitKey();
    
    LeafNode *new_leaf_node = new LeafNode(tree);
    new_leaf_node->next = this->next;
    if (this->next != NULL)
        this->next->prev = new_leaf_node;
    this->next = new_leaf_node;
    new_leaf_node->prev = this;
    *pNext = this->next->pPointer;
    // persist();
    for(int i = 0; i < LEAF_DEGREE * 2;i++)
        if(getBit(i) && kv[i].k >= split_key)
        {
            new_leaf_node->insertNonFull(kv[i].k, kv[i].v);
            //remove entry
            clearBit(bitmap, i);
            this->n--;
        }
    newChild->key = split_key;
    newChild->node = new_leaf_node;
    return newChild;
}

// use to find a mediant key and delete entries less then middle
// called by the split func to generate new leaf-node
// qsort first then find
Key LeafNode::findSplitKey() {
    Key midKey = 0;
    // DONE
    int cnt = 0;
    KeyValue *sort_kv = new KeyValue[LEAF_DEGREE * 2];
    for(int i = 0; i < LEAF_DEGREE*2; i++)
        if(getBit(i))
            sort_kv[cnt++] = kv[i];
    sort(sort_kv, sort_kv + cnt,[](KeyValue x, KeyValue y){return x.k < y.k;});

    midKey = sort_kv[n/2].k;
    return midKey;
}

// get the targte bit in bitmap
// TIPS: bit operation
int LeafNode::getBit(const int& idx) {
    // DONE
    int nshift = 7-(idx%8);
    return ((bitmap[idx/8] >> nshift) & 0x01);
}

Key LeafNode::getKey(const int& idx) {
    return this->kv[idx].k;
}

Value LeafNode::getValue(const int& idx) {
    return this->kv[idx].v;
}

PPointer LeafNode::getPPointer() {
    return this->pPointer;
}

// remove an entry from the leaf
// if it has no entry after removement return TRUE to indicate outer func to delete this leaf.
// need to call PAllocator to set this leaf free and reuse it
bool LeafNode::remove(const Key& k, const int& index, InnerNode* const& parent, bool &ifDelete) {
    bool ifRemove = false;
    // done

    int slot;
    for (slot = 0; slot < 2 * degree; slot++) {
        Key currentKey = kv[slot].k;
        if(getBit(slot) == 1 && fingerprints[slot] == keyHash(k) && currentKey == k) {
            ifRemove = true;
            break;
        }
    }
    if (ifRemove) {
        ifDelete = false;
        clearBit(bitmap, slot);
        n--;
        if (n == 0) {
            ifDelete = true;
            PAllocator::getAllocator()->freeLeaf(this->pPointer);
        }
        persist();
    }
    return ifRemove;
}

// update the target entry
// return TRUE if the update succeed
bool LeafNode::update(const Key& k, const Value& v) {
    bool ifUpdate = false;
    // TODO:
    int slot;
    for (slot = 0; slot < 2 * degree; slot++) {
        Key currentKey = kv[slot].k;
        if(getBit(slot) == 1 && fingerprints[slot] == keyHash(k) && currentKey == k) {
            kv[slot].v = v;
            ifUpdate = true;
            persist();
            break;
        }
    }
    return ifUpdate;
}

// if the entry can not be found, return the max Value
Value LeafNode::find(const Key& k) {
    // done
    int slot;
    for (slot = 0; slot < 2 * degree; slot++) {
        Key currentKey = kv[slot].k;
        if(getBit(slot) == 1 && fingerprints[slot] == keyHash(k) && currentKey == k)
            return kv[slot].v;
    }
    return MAX_VALUE;
}

// find the first empty slot
int LeafNode::findFirstZero() {
    // DONE
    for(int i = 0; i < LEAF_DEGREE*2; i++)
    {   
        if ( getBit(i) == 0)
            return i;
    }
    return -1;
}

// persist the entire leaf
// use PMDK
void LeafNode::persist() {
    // TODO:
    size_t len =LEAF_GROUP_HEAD+LEAF_GROUP_AMOUNT*calLeafSize();
    pmem_persist(pmem_addr, calLeafSize());
}

// call by the ~FPTree(), delete the whole tree
void FPTree::recursiveDelete(Node* n) {
    if (n->isLeaf) {
        delete n;
    } else {
        for (int i = 0; i < ((InnerNode*)n)->nChild; i++) {
            recursiveDelete(((InnerNode*)n)->childrens[i]);
        }
        delete n;
    }
}

FPTree::FPTree(uint64_t t_degree) {
    FPTree* temp = this;
    this->root = new InnerNode(t_degree, temp, true);
    this->degree = t_degree;
    bulkLoading();
}

FPTree::~FPTree() {
    recursiveDelete(this->root);
}

// get the root node of the tree
InnerNode* FPTree::getRoot() {
    return this->root;
}

// change the root of the tree
void FPTree::changeRoot(InnerNode* newRoot) {
    this->root = newRoot;
}

void FPTree::insert(Key k, Value v) {
    if (root != NULL) {
        root->insert(k, v);
    }
}

bool FPTree::remove(Key k) {
    if (root != NULL) {
        bool ifDelete = false;
        InnerNode* temp = NULL;
        return root->remove(k, -1, temp, ifDelete);
    }
    return false;
}

bool FPTree::update(Key k, Value v) {
    if (root != NULL) {
        return root->update(k, v);
    }
    return false;
}

Value FPTree::find(Key k) {
    if (root != NULL) {
        return root->find(k);
    }
}

// call the InnerNode and LeafNode print func to print the whole tree
// TIPS: use Queue
void FPTree::printTree() {
    // done
    if (root == NULL) return;
    queue<Node *> q;
    Node * pN;
    Node * last = root;
    q.push(root);
    while (!q.empty()) {
        pN = q.front();
        q.pop();
        pN->printNode();
        if (!pN->ifLeaf()) {
            InnerNode * pIn = (InnerNode *)pN;
            for (int i = 0; i < pIn->nChild; i++) {
                if (pIn->childrens[i] != NULL)
                    q.push(pIn->childrens[i]);
            }
            if (pN == last) {
                cout << endl;
                last = pIn->childrens[pIn->nChild - 1];
            }
        }    
    }
    cout << endl;
}

// bulkLoading the leaf files and reload the tree
// need to traverse leaves chain
// if no tree is reloaded, return FALSE
// need to call the PALlocator
bool FPTree::bulkLoading() {
    // TODO:
    PAllocator *pAllocator = PAllocator::getAllocator();
    //1. read data file : not exist -> return false
    //2. exitst -> call  PAllocator -> leave sort
    //3. insert return true


    PPointer start = pAllocator->getStartPointer();
    if(!pAllocator->ifLeafUsed(start)) {
        // cout << "false" << endl;
        return false;
    }

    uint64_t leaf_amount = LEAF_GROUP_AMOUNT*(pAllocator->getMaxFileId() - 1) - pAllocator->getFreeNum();
    for(uint64_t i = 0; i < leaf_amount; i++){
        LeafNode *leaf_temp = new LeafNode(start, this);
        KeyNode key_temp;
        key_temp.key = leaf_temp->getKey(0);
        for (int j = 1; j < leaf_temp->n; j++) {
            if (key_temp.key > leaf_temp->getKey(j))
                key_temp.key = leaf_temp->getKey(j);
        }
        key_temp.node = leaf_temp;
        root->insertLeaf(key_temp);

        start = *(leaf_temp->pNext);

    }

    return true;
}
