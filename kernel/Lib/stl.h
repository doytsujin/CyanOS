

template <typename T> struct RemoveReference {
	typedef T Type;
};
template <class T> struct RemoveReference<T&> {
	typedef T Type;
};
template <class T> struct RemoveReference<T&&> {
	typedef T Type;
};

template <typename T> inline T&& move(T& t)
{
	return static_cast<T&&>(t);
}
