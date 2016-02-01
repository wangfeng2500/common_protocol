/*
 * singleton.h
 *
 *  Created on: 2016-2-1
 *      Author: wangfeng
 */

#ifndef SINGLETON_H_
#define SINGLETON_H_

template <typename T>
class CSingleton {
public:
	static T* instance();
};

template <typename T>
T* CSingleton<T>:: instance()
{
	static T _instance;
	return &_instance;
}

#endif /* SINGLETON_H_ */
