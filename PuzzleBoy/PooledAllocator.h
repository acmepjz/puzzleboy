#pragma once

/** \file
*/

#include <stdlib.h>
#include <assert.h>

template <class T>
struct PooledAllocatorChunk{
	PooledAllocatorChunk* next;
	intptr_t size;
	T item[1];
};

template <class T>
class PooledAllocatorBase{
public:
	PooledAllocatorBase():chunks(NULL),m_size(0),m_capacity(0){
	}
	~PooledAllocatorBase(){
		clear();
	}
	void clear(){
		while(chunks){
			PooledAllocatorChunk<T> *nextChunk=chunks->next;
			free(chunks);
			chunks=nextChunk;
		}

		m_size=0;
		m_capacity=0;
	}
	intptr_t size() const{
		return m_size;
	}
	intptr_t capacity() const{
		return m_capacity;
	}
	void reserve(intptr_t size){
		while(size>m_capacity){
			intptr_t i=0,i2=0;
			if(chunks){
				i=chunks->size;
				i2=i*2;
			}else{
				i=16;
				i2=1024;
			}
			if(i<size-m_capacity) i=size-m_capacity;
			if(i>i2) i=i2;
			addChunk(i);
		}
	}
protected:
	PooledAllocatorChunk<T>* chunks;
	intptr_t m_size,m_capacity;

	//internal function
	virtual PooledAllocatorChunk<T>* addChunk(intptr_t size){
		PooledAllocatorChunk<T>* newChunk=NULL;

		for(;;){
			if((newChunk=(PooledAllocatorChunk<T>*)malloc(sizeof(void*)
				+sizeof(intptr_t)+sizeof(T)*size))!=NULL) break;

			//out of memory?
			size/=2;
			if(size<1024) abort();
		}

		newChunk->next=chunks;
		newChunk->size=size;

		//assume it is 8-byte align
		assert((intptr_t(newChunk->item)&0x7)==0x0);
		//assume sizeof(T) is multiple of 8
		assert((sizeof(T)&0x7)==0x0);

		m_capacity+=size;

		chunks=newChunk;
		return newChunk;
	}

	//ad-hoc fix for stupid GCC
	PooledAllocatorChunk<T>* do_addChunk(intptr_t size){
		return addChunk(size);
	}
};

/** A simple memory pooled chunk allocator.
\note sizeof(T) should be >=sizeof(void*)
\note constructor and destructor of T won't be called.
*/

template <class T>
class PooledAllocator:public PooledAllocatorBase<T>{
public:
	PooledAllocator():firstUnused(NULL){
	}
	void clear(){
		PooledAllocatorBase<T>::clear();

		firstUnused=NULL;
	}
	/** Allocate an item. */
	T* allocate(){
		if(firstUnused==NULL){
			PooledAllocatorBase<T>::reserve(
				PooledAllocatorBase<T>::m_capacity<16?
				16:PooledAllocatorBase<T>::m_capacity*2);
		}

		T *node=firstUnused;
		firstUnused=*(T**)node;

		PooledAllocatorBase<T>::m_size++;

		return node;
	}
	/** Free an item.
	\note It should be in current allocator, and shouldn't be freed twice, or undefined behavior occurs.
	*/
	void deallocate(T* node){
		if(node==NULL) return;

		*(T**)node=firstUnused;
		firstUnused=node;

		PooledAllocatorBase<T>::m_size--;
	}
protected:
	T* firstUnused;

	//internal function
	virtual PooledAllocatorChunk<T>* addChunk(intptr_t size) override{
		assert(sizeof(T)>=sizeof(T*));
		PooledAllocatorChunk<T>* newChunk=PooledAllocatorBase<T>::addChunk(size);
		size=newChunk->size;

		for(intptr_t i=0;i<size-1;i++){
			*(T**)(newChunk->item+i)=newChunk->item+(i+1);
		}
		*(T**)(newChunk->item+(size-1))=firstUnused;
		firstUnused=newChunk->item;

		return newChunk;
	}
};

template <class T>
class AllocateOnlyPooledAllocator:public PooledAllocatorBase<T>{
public:
	/// Allocate an item.
	T* allocate(){
		if(PooledAllocatorBase<T>::m_size>=PooledAllocatorBase<T>::m_capacity){
			reserve(
				PooledAllocatorBase<T>::m_capacity<16?
				16:PooledAllocatorBase<T>::m_capacity*2);
		}

		//FIXME: need specified reserve function to make it work
		T *node=PooledAllocatorBase<T>::chunks->item+(PooledAllocatorBase<T>::m_capacity
			-(++PooledAllocatorBase<T>::m_size));

		return node;
	}
	//we need this specified reserve function
	void reserve(intptr_t size){
		intptr_t i=0,i2=0;
		if(PooledAllocatorBase<T>::chunks){
			i=PooledAllocatorBase<T>::chunks->size;
			i2=i*2;
		}else{
			i=16;
			i2=1024;
		}
		if(i<size-PooledAllocatorBase<T>::m_capacity)
			i=size-PooledAllocatorBase<T>::m_capacity;
		if(i>i2) i=i2;
		PooledAllocatorBase<T>::do_addChunk(i);
	}
};
