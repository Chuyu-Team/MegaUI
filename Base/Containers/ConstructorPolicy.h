#pragma once

namespace YY
{
    namespace Base
    {
        namespace Containers
        {
            template<typename _Type>
            struct ConstructorPolicy
            {
                static void Constructor(_Type* _pFirst, _Type* _pLast)
                {
                    if /*constexpr*/ (std::is_trivially_constructible<_Type>::value)
                    {
                        memset(_pFirst, 0, (_pLast - _pFirst) * sizeof(*_pFirst));
                    }
                    else
                    {
                        for (; _pFirst != _pLast; ++_pFirst)
                        {
                            new (_pFirst) _Type {};
                        }
                    }
                }

                static void MoveConstructor(_Type* _pDst, _Type* _pFirst, _Type* _pLast)
                {
                    if /*constexpr*/ (std::is_trivially_constructible<_Type>::value)
                    {
                        memcpy(_pDst, _pFirst, (_pLast - _pFirst) * sizeof(*_pFirst));
                    }
                    else
                    {
                        for (; _pFirst != _pLast; ++_pFirst, ++_pDst)
                        {
                            new (_pDst) _Type(std::move(*_pFirst));
                        }
                    }
                }

                static void MoveConstructor(_Type* _pDst, _Type* _pSrc, size_t _uCount)
                {
                    MoveConstructor(_pDst, _pSrc, _pSrc + _uCount);
                }

                static void CopyConstructor(_Type* _pDst, const _Type* _pFirst, const _Type* _pLast)
                {
                    if /*constexpr*/ (std::is_trivially_constructible<_Type>::value)
                    {
                        memcpy(_pDst, _pFirst, (_pLast - _pFirst) * sizeof(*_pFirst));
                    }
                    else
                    {
                        for (; _pFirst != _pLast; ++_pFirst, ++_pDst)
                        {
                            new (_pDst) _Type(*_pFirst);
                        }
                    }
                }

                static void CopyConstructor(_Type* _pDst, const _Type* _pSrc, size_t _uCount)
                {
                    return CopyConstructor(_pDst, _pSrc, _pSrc + _uCount);
                }

                static void Destructor(_Type* _pFirst, _Type* _pLast)
                {
                    if /*constexpr*/ (std::is_trivially_destructible<_Type>::value)
                    {
                        // �����ƽ̹�ģ���ôʲôҲ����
                        return;
                    }
                    else
                    {
                        for (; _pFirst != _pLast; ++_pFirst)
                        {
                            _pFirst->~_Type();
                        }
                    }
                }

                static void Destructor(_Type* _pItems, size_t _uCount)
                {
                    Destructor(_pItems, _pItems + _uCount);
                }

                static void Copy(_Type* _pDst, const _Type* _pFirst, const _Type* _pLast)
                {
                    if /*constexpr*/ (std::is_trivially_copyable<_Type>::value)
                    {
                        memcpy(_pDst, _pFirst, (_pLast - _pFirst) * sizeof(*_pFirst));
                    }
                    else
                    {
                        for (; _pFirst != _pLast; ++_pDst, ++_pFirst)
                        {
                            *_pDst = *_pFirst;
                        }
                    }
                }

                static void Copy(_Type* _pDst, const _Type* _pSrc, size_t _uCount)
                {
                    return Copy(_pDst, _pSrc, _pSrc + _uCount);
                }

                static void Move(_Type* _pDst, _Type* _pFirst, _Type* _pLast)
                {
                    if /*constexpr*/ (std::is_trivially_copyable<_Type>::value)
                    {
                        memmove(_pDst, _pFirst, (_pLast - _pFirst) * sizeof(*_pFirst));
                    }
                    else
                    {
                        if (_pDst == _pFirst)
                        {
                            // ��ȫһ��������ɶ������
                        }
                        else if (_pFirst > _pDst && _pDst < _pLast)
                        {
                            // �����������ǽ��еߵ��ƶ���
                            auto _pDstLast = _pDst + (_pLast - _pFirst);

                            while (_pFirst != _pLast)
                            {
                                --_pLast;
                                --_pDstLast;
                                *_pDstLast = std::move(*_pLast);
                            }
                        }
                        else
                        {
                            for (; _pFirst != _pLast; ++_pDst, ++_pFirst)
                            {
                                *_pDst = std::move(*_pFirst);
                            }
                        }
                    }
                }
            };
        } // namespace Containers
    } // namespace Base
    
    using namespace YY::Base::Containers;

} // namespace YY
