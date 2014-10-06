#pragma once

#include <string.h>

template <class T>
struct SimpleHashAVLTreeNode{
public:
	inline SimpleHashAVLTreeNode* get_lson() const{
		return (SimpleHashAVLTreeNode*)(lson&~0x7);
	}

	inline void set_lson_only(SimpleHashAVLTreeNode* p){
		lson=((intptr_t)p)|(lson&0x7);
	}

	inline void set_lson(SimpleHashAVLTreeNode* p,char balanceFactor){
		//assume p is 8-byte aligned
		lson=((intptr_t)p)|(balanceFactor&0x7);
	}

	inline char getBalanceFactor() const{
		return char(char(lson)<<5)>>5;
	}

	inline void setBalanceFactor(char balanceFactor){
		lson=(lson&~0x7)|(balanceFactor&0x7);
	}
private:
	intptr_t lson; //balance factor is low 3 bits of lson
public:
	SimpleHashAVLTreeNode *rson;
	T data;
};

//simple hash AVL tree.
//T: the type.
//N: 2^N is size of hash table.
//A: memory allocator, e.g. PooledAllocator<SimpleHashAVLTreeNode<T> >
//or AllocateOnlyPooledAllocator<SimpleHashAVLTreeNode<T> >
//note: T must implements `int Compare(const T& other) const` (should only compare keys, returns -1,0,1)
//and `int HashValue() const` (returns hash value; always return 0 is okay)

template <class T,unsigned int N,class A>
class SimpleHashAVLTreeBase{
public:
	enum{
		HashTableSize=1<<N,
	};
	A alloc;
protected:
	SimpleHashAVLTreeNode<T>* HashTable[HashTableSize];
private:
	static inline void pRotateL(SimpleHashAVLTreeNode<T>*& lp){
		SimpleHashAVLTreeNode<T> *lson=lp;
		lp=lson->rson;
		lson->rson=lp->get_lson();
		lp->set_lson(lson,0);
		//lp->nBalanceFactor=0;
		lson->setBalanceFactor(0);
	}
	static inline void pRotateR(SimpleHashAVLTreeNode<T>*& lp){
		SimpleHashAVLTreeNode<T> *rson=lp;
		lp=rson->get_lson();
		rson->set_lson(lp->rson,0);
		lp->rson=rson;
		lp->setBalanceFactor(0);
		//rson->nBalanceFactor=0;
	}
	static inline void pRotateLR(SimpleHashAVLTreeNode<T>*& lp){
		SimpleHashAVLTreeNode<T> *rson=lp,*lson=lp->get_lson();
		lp=lson->rson;
		lson->rson=lp->get_lson();
		char i=lp->getBalanceFactor();
		lp->set_lson(lson,0);
		lson->setBalanceFactor(i<=0?0:-1);
		rson->set_lson(lp->rson,i<0?1:0);
		lp->rson=rson;
		//rson->nBalanceFactor=i<0?1:0;
		//lp->nBalanceFactor=0;
	}
	static inline void pRotateRL(SimpleHashAVLTreeNode<T>*& lp){
		SimpleHashAVLTreeNode<T> *lson=lp,*rson=lp->rson;
		lp=rson->get_lson();
		SimpleHashAVLTreeNode<T> *tmp=lp->rson;
		lp->rson=rson;
		char i=lp->getBalanceFactor();
		rson->set_lson(tmp,i>=0?0:1);
		lson->rson=lp->get_lson();
		lp->set_lson(lson,0);
		lson->setBalanceFactor(i>0?-1:0);
		//lp->nBalanceFactor=0;
	}
public:
	SimpleHashAVLTreeBase(){
		memset(HashTable,0,sizeof(HashTable));
	}
	intptr_t size() const{
		return alloc.size();
	}
	void clear(){
		alloc.clear();
		memset(HashTable,0,sizeof(HashTable));
	}
	//return value: true means found, otherwise not found
	bool find_or_insert(const T& data,T** ret=0){
		unsigned int h=((unsigned int)data.HashValue())&(unsigned int)(HashTableSize-1);
		SimpleHashAVLTreeNode<T> *p,*pr=0;
		//An AVL tree's height is strictly less than 1.44*log2(n+2)-0.328, see Wikipedia
		SimpleHashAVLTreeNode<T> *st[sizeof(void*)*12];
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
		p->set_lson(0,0);
		p->rson=0;
		p->data=data;
		if(ret) *ret=&(p->data);
		if(pr){
			if(d<0) pr->set_lson_only(p);
			else pr->rson=p;

			while(stIndex>0){
				pr=st[--stIndex];

				d=pr->getBalanceFactor()+((p==pr->get_lson())?-1:1);
				pr->setBalanceFactor(d);

				if(d==0) break;
				else if(d==1||d==-1) p=pr;
				else{
					d=d<0?-1:1;
					if(p->getBalanceFactor()==d){
						if(d<0) pRotateR(pr); else pRotateL(pr);
					}else{
						if(d<0) pRotateLR(pr); else pRotateRL(pr);
					}
					break;
				}
			}

			if(stIndex==0) HashTable[h]=pr;
			else{
				p=st[stIndex-1];
				if(p->data.Compare(pr->data)>0) p->set_lson_only(pr);
				else p->rson=pr;
			}
		}else{
			HashTable[h]=p;
		}

		return false;
	}
};

//template <class T> class PooledAllocator;
template <class T> class AllocateOnlyPooledAllocator;

/*template <class T,unsigned int N>
class SimpleHashAVLTree:public SimpleHashAVLTreeBase<T,N,PooledAllocator<SimpleHashAVLTreeNode<A> > >{
};*/

template <class T,unsigned int N>
class AllocateOnlyHashAVLTree:public SimpleHashAVLTreeBase<T,N,AllocateOnlyPooledAllocator<SimpleHashAVLTreeNode<T> > >{
};
