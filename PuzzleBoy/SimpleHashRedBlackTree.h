#pragma once

#include <string.h>

template <class T>
struct SimpleHashRedBlackTreeNode{
public:
	inline SimpleHashRedBlackTreeNode* get_lson() const{
		return (SimpleHashRedBlackTreeNode*)(lson&~0x1);
	}

	inline void set_lson_only(SimpleHashRedBlackTreeNode* p){
		lson=((intptr_t)p)|(lson&0x1);
	}

	inline void set_lson(SimpleHashRedBlackTreeNode* p,char color){
		//assume p is 8-byte aligned
		lson=((intptr_t)p)|(color&0x1);
	}

	inline bool isColorRed() const{
		return (int(lson)&0x1)==0;
	}

	inline bool isColorBlack() const{
		return (int(lson)&0x1)!=0;
	}

	inline void setColorRed(){
		lson&=~0x1;
	}

	inline void setColorBlack(){
		lson|=0x1;
	}
private:
	intptr_t lson; //color is lowest bit of lson
public:
	SimpleHashRedBlackTreeNode *rson;
	T data;
};

//simple hash red-black tree.
//according to Wikipedia it has faster insertion/deletion but slower look-up operation.

template <class T,unsigned int N,class A>
class SimpleHashRedBlackTreeBase{
public:
	enum{
		HashTableSize=1<<N,
		RED=0,
		BLACK=1,
	};
protected:
	A alloc;
	SimpleHashRedBlackTreeNode<T>* HashTable[HashTableSize];
public:
	SimpleHashRedBlackTreeBase(){
		memset(HashTable,0,sizeof(HashTable));
	}
	void clear(){
		alloc.clear();
		memset(HashTable,0,sizeof(HashTable));
	}
	//return value: true means found, otherwise not found
	bool find_or_insert(const T& data,T** ret=0){
		unsigned int h=((unsigned int)data.HashValue())&(unsigned int)(HashTableSize-1);
		SimpleHashRedBlackTreeNode<T> *p,*pr=0;
		//A red-black tree's height is at most 2*log2(n+1), see Wikipedia
		SimpleHashRedBlackTreeNode<T> *st[sizeof(void*)*16];
		int stIndex=0;
		char d=0;

		//find node
		for(p=HashTable[h];p;){
			if(!(d=data.Compare(p->data))){
				if(ret) *ret=&(p->data);
				return true;
			}
			st[stIndex++]=pr=p;
			p=d<0?p->get_lson():p->rson;
		}

		//not found, add item
		p=alloc.allocate();

		//Copied from Wikipedia
		//set newly inserted node color RED
		p->set_lson(0,RED);
		p->rson=0;
		p->data=data;

		if(ret) *ret=&(p->data);

		//insert
		if(pr){
			if(d<0) pr->set_lson_only(p);
			else pr->rson=p;
		}

		//p: current node, same as st[stIndex], except for the beginning
		//pr: parent of current node, same as st[stIndex-1]
		for(;;){
			//insert_case1
			if(!pr){
				//The current node is at the root of the tree
				p->setColorBlack();
				HashTable[h]=p; //???
				break;
			}

			//insert_case2
			if(pr->isColorBlack()) break; //Tree is still valid

			//insert_case3
			//no need to check stIndex because pr can't be root because root is always BLACK
			SimpleHashRedBlackTreeNode<T> *g=st[stIndex-2]; //grandparent
			SimpleHashRedBlackTreeNode<T> *u=(g->rson==pr)?g->get_lson():g->rson; //uncle
			if(u && u->isColorRed()){
				pr->setColorBlack();
				u->setColorBlack();
				g->setColorRed();

				stIndex-=2;
				p=g;
				pr=(stIndex>=1)?st[stIndex-1]:0; //parent
				continue;
			}

			//insert_case4
			if(p==pr->rson && pr==g->get_lson()){
				//rotate left
				SimpleHashRedBlackTreeNode<T> *saved_p=pr, *saved_left_n=p->get_lson();
				g->set_lson(p,BLACK /* ??? */); 
				p->set_lson(saved_p,RED /* ??? */);
				saved_p->rson=saved_left_n;

				pr=p;
				p=saved_p;
			}else if(pr==g->rson && p==pr->get_lson()){
				//rotate right
				SimpleHashRedBlackTreeNode<T> *saved_p=pr, *saved_right_n=p->rson;
				g->rson=p; 
				p->rson=saved_p;
				saved_p->set_lson(saved_right_n,RED /* ??? */);

				pr=p;
				p=saved_p;
			}

			//insert_case5
			//TODO: optimize and clean
			pr->setColorBlack();
			SimpleHashRedBlackTreeNode<T> *old_g=g;
			if(p==pr->rson){
				//rotate left
				g=old_g->rson;
				old_g->setColorRed();
				old_g->rson=g->get_lson();
				g->set_lson(old_g,BLACK /* ??? */);
			}else{
				//rotate right
				g=old_g->get_lson();
				old_g->set_lson(g->rson,RED);
				g->rson=old_g;
			}

			//update parent of g
			if(stIndex>=3){
				SimpleHashRedBlackTreeNode<T> *gg=st[stIndex-3];
				if(gg->rson==old_g) gg->rson=g;
				else gg->set_lson_only(g);
			}else{
				//update root node
				HashTable[h]=g;
			}

			//over
			break;
		}

		return false;
	}
};

//template <class T> class PooledAllocator;
template <class T> class AllocateOnlyPooledAllocator;

/*template <class T,unsigned int N>
class SimpleHashRedBlackTree:public SimpleHashRedBlackTreeBase<T,N,PooledAllocator<SimpleHashRedBlackTreeNode<A> > >{
};*/

template <class T,unsigned int N>
class AllocateOnlyHashRedBlackTree:public SimpleHashRedBlackTreeBase<T,N,AllocateOnlyPooledAllocator<SimpleHashRedBlackTreeNode<T> > >{
};
