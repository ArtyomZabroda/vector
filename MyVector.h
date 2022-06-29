#pragma once
#include <algorithm>
#include <iostream>

namespace zabroda {

	template<typename T>
	struct vector_base {
		using size_type = unsigned int;
		T* elem; // начало выделенной памяти
		T* space; // конец последовательности элементов
		T* last; // конец выделенной памяти

		vector_base(size_type n, size_t s)
			: elem{ static_cast<T*>( ::operator new(sizeof(T) * n) ) }, space{ elem + s }, last{ elem + n } {}
		~vector_base() { ::operator delete[](elem); }

		vector_base(const vector_base&) = delete;
		vector_base& operator=(const vector_base&) = delete;

		vector_base(vector_base&& a) noexcept
			: elem{a.elem}, space{a.space}, last{a.space}
		{
			a.elem = a.space = a.last = nullptr;
		}

		vector_base& operator=(vector_base&& a) noexcept
		{
			::operator delete[](elem);
			elem = a.elem;
			space = a.space;
			last = a.last ;
			a.elem = a.space = a.last = nullptr;
			return *this;
		}
	};

	template<typename T>
	class vector {
	public:
		using size_type = unsigned int;
		using iterator = T*;
		using const_iterator = const T*;

		iterator begin() { return vb.elem; }
		iterator end() { return vb.space; }
		const_iterator begin() const { return vb.elem; }
		const_iterator end() const { return vb.space; }

		explicit vector(size_type n = 0, const T& val = T{});
		vector(std::initializer_list<T> init);

		vector(const vector& vec);
		vector& operator=(const vector& vec);

		vector(vector&& vec) noexcept;
		vector& operator=(vector&& vec) noexcept;

		~vector();

		size_type size() const { return vb.space - vb.elem; };
		size_type capacity() const { return vb.last - vb.elem; };
		void reserve(size_type n); // увеличение выделенной памяти ( увеличение capacity() )

		void resize(size_type n, const T& = {}); // изменить кол-во элементов
		void push_back(const T&); // добавить элемент в конец вектора
		void insert(size_type n, const T& = {});
		void remove(size_type n);
		void clear() { resize(0); } // очистить вектор

		T& operator [] (size_type i) { return vb.elem[i]; }
		T operator [] (size_type i) const { return vb.elem[i]; }

		friend std::ostream& operator << (std::ostream& out, const vector& vec)
		{
			for (const auto& a : vec)
				out << a << ' ';
			return out;
		}

		friend std::istream& operator >> (std::istream& in, vector& vec)
		{
			int n;
			std::cout << "Size = ";
			std::cin >> n;
			T a;
			for (int i{}; i < n && in >> a; ++i) {
				vec.push_back(a);
			}
			return in;
		}

		vector operator+(const vector& vec);
		vector& operator+=(const vector& vec);
		vector operator+(const T x);

	private:
		vector_base<T> vb;
		void destroy_elements();
	};

	template<typename For, typename T>
	void uninit_fill(For beg, For end, const T& x)
	{
		For p;
		try {
			for (p = beg; p != end; ++p)
				new(static_cast<void*>(&*p)) T(x); // Конструируем *p с помощью копии элемента x
		}
		catch (...) { // Если не получилось сконструировать копию, то вызываем деструкторы всех элементов
			for (For q = beg; q != p; ++q)
				(&*q)->~T();
			throw; // Перебрасываем исключение в функцию выше
		}
	}

	// Самописнй аналог std::uninitialized_copy()
	template<typename From, typename To>
	void uninit_copy(From beg, From end, To dist)
	{
		using Value = typename std::iterator_traits<To>::value_type;
		From p;
		try {
			for (p = beg; p != end; ++p, ++dist)
				new(const_cast<void*>(static_cast<const void*>(&*dist))) Value(*p);
		}
		catch (...) { // Если не получилось сконструировать копию, то вызываем деструкторы всех элементов
			for (From q = beg; q != p; ++q)
				(&(*q))->~Value();
			throw; // Перебрасываем исключение в функцию выше
		}
	}

	
	template<typename From, typename To>
	void uninit_move(From beg, From end, To dist)
	{
		using Value = typename std::iterator_traits<To>::value_type;
		for (beg; beg != end; ++beg, ++dist) {
			new(static_cast<void*>(&*dist)) Value{ std::move(*beg) };
			beg->~Value();
		}
		
	}

	template<typename T>
	inline vector<T>::vector(size_type n, const T& val)
		: vb{ n, n }
	{
		uninit_fill(vb.elem, vb.elem + n, val);
	}

	template<typename T>
	inline vector<T>::vector(std::initializer_list<T> init)
		: vb{ init.size(), init.size() }
	{
		uninit_copy(init.begin(), init.end(), vb.elem);
	}

	template<typename T>
	inline vector<T>::vector(const vector& vec)
		: vb{ vec.capacity(), vec.size() }
	{
		uninit_copy(vec.begin(), vec.end(), vb.elem);
	}

	template<typename T>
	inline vector<T>& vector<T>::operator=(const vector& vec)
	{
		if (capacity() < vec.size()) {
			vector temp{ vec };
			std::swap(*this, temp);
			return *this;
		}

		if (this == &vec) return *this; // Оптимизация самоприсвоения (vec = vec)

		size_type sz = size();
		size_type vecsz = vec.size();
		if (vecsz <= sz) {
			std::copy(vec.begin(), vec.begin() + vecsz, vb.elem);
			for (T* p = vb.elem + vecsz; p != vb.space; ++p) // уничтожаем неиспользующуюся выделенную память
				p->~T();
		}
		else {
			std::copy(vec.begin(), vec.begin() + sz, vb.elem);
			uninit_copy(vec.begin() + sz, vec.end(), vb.space);
		}
	}

	template<typename T>
	inline vector<T>::vector(vector&& vec) noexcept
		: vb{ std::move(vec.vb) } {}

	template<typename T>
	inline vector<T>& vector<T>::operator=(vector&& vec) noexcept
	{
		clear();
		std::swap(this->vb, vec.vb);
		return *this;
	}

	template<typename T>
	inline vector<T>::~vector()
	{
		destroy_elements();
	}

	template<typename T>
	inline void vector<T>::reserve(size_type n)
	{
		if (n <= capacity()) return; // В этом случае нам хватает места
		vector_base<T> b(n, size());
		uninit_move(vb.elem, vb.elem + size(), b.elem);
		std::swap(vb, b);
	}

	template<typename In>
	void destroy(In b, In e)
	{
		using Value = typename std::iterator_traits<In>::value_type;
		for (; b != e; ++b) // destroy [b:e)
			b->~Value();
	}

	template<typename T>
	inline void vector<T>::resize(size_type n, const T& val)
	{
		reserve(n);
		if (size() < n)
			uninit_fill(vb.elem + size(), vb.elem + n, val);
		else
			destroy(vb.elem + n, vb.elem + size());
		vb.space = vb.last = vb.elem + n;
	}

	template<typename T>
	inline void vector<T>::push_back(const T& val)
	{
		if (capacity() == size())
			reserve(size() ? 2 * size() : 8);
		new (static_cast<void*>(&vb.elem[size()])) T(val); // конструирую объект в конце
		++vb.space; // увеличиваю кол-во сконструированных элементов на 1
	}

	template<typename T>
	inline void vector<T>::insert(size_type n, const T& val)
	{
		if (capacity() == size())
			reserve(size() ? 2 * size() : 8);
		new (static_cast<void*>(&vb.elem[size()])) T(val);
		for (size_type i{ size()}; i > n; --i) {
			(*this)[i] = (*this)[i - 1];
		}
		(*this)[n] = val;
		++vb.space;
	}

	template<typename T>
	inline void vector<T>::remove(size_type n)
	{
		for (size_type i{ n }; i < size(); ++i) {
			(*this)[i] = (*this)[i + 1];
		}
		--vb.space;
	}


	template<typename T>
	inline vector<T> vector<T>::operator+(const vector& vec)
	{
		bool b = size() < vec.size();
		size_type smsz = b ? size() : vec.size();
		size_type sz = b ? vec.size() : size();
		vector new_vec(sz);
		for (size_type i{}; i < smsz; ++i)
			new_vec[i] = (*this)[i] + vec[i];
		for (size_type i{smsz}; i < sz; ++i)
			new_vec[i] = b ? (vec)[i] : (*this)[i];

		return new_vec;
	}

	template<typename T>
	inline vector<T>& vector<T>::operator+=(const vector& vec)
	{
		*this = *this + vec;
		return *this;
	}

	template<typename T>
	inline vector<T> vector<T>::operator+(const T x)
	{
		size_type sz = size();
		vector vec(sz);
		for (size_type i{}; i < sz; ++i) {
			vec[i] = ((*this)[i]) + x;
		}
		return vec;
	}

	template<typename T>
	inline void vector<T>::destroy_elements()
	{
		for (T* p = vb.elem; p != vb.space; ++p)
			p->~T();
		vb.space = vb.elem;
	}


}