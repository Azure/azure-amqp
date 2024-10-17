// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Reflection;
    using System.Runtime.Serialization;

    static class ReflectionExtentions
    {
        static readonly Type[] delegateParamsType = { typeof(object), typeof(object[]) };

        delegate TReturn FuncRef<TTarget, TReturn>(ref TTarget target);

        // FieldInfo has to use reflection so it is slow.
        // Users need to convert fields to properties when upgrade to netstandard2.0
        public static Func<object, object> CreateGetter(this FieldInfo fieldInfo)
        {
            return obj => fieldInfo.GetValue(obj);
        }

        public static Action<object, object> CreateSetter(this FieldInfo fieldInfo)
        {
            return (obj, val) => fieldInfo.SetValue(obj, val);
        }

        [RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
        public static Func<object, object> CreateGetter(this PropertyInfo propertyInfo)
        {
            MethodInfo getMethod = propertyInfo.GetGetMethod(true);
            MethodInfo m1 = typeof(ReflectionExtentions).GetMethod("CreateFuncDelegate", BindingFlags.Static | BindingFlags.NonPublic);
            MethodInfo m2 = m1.MakeGenericMethod(propertyInfo.DeclaringType, propertyInfo.PropertyType);
            return (Func<object, object>)m2.Invoke(null, new object[] { getMethod });
        }

        [RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
        public static Action<object, object> CreateSetter(this PropertyInfo propertyInfo, bool requiresSetter)
        {
            if (requiresSetter && propertyInfo.DeclaringType.IsValueType)
            {
                throw new NotSupportedException($"{propertyInfo.Name}: {propertyInfo.DeclaringType.Name} is a value type.");
            }

            MethodInfo setMethod = propertyInfo.GetSetMethod(true);
            if (setMethod == null)
            {
                if (requiresSetter)
                {
                    throw new SerializationException("Property annotated with AmqpMemberAttribute must have a setter.");
                }
                else
                {
                    return null;
                }
            }

            MethodInfo m1 = typeof(ReflectionExtentions).GetMethod("CreateActionDelegate", BindingFlags.Static | BindingFlags.NonPublic);
            MethodInfo m2 = m1.MakeGenericMethod(propertyInfo.DeclaringType, propertyInfo.PropertyType);
            return (Action<object, object>)m2.Invoke(null, new object[] { setMethod });
        }

        public static MethodDelegate CreateMethod(this ConstructorInfo constructorInfo)
        {
            throw new NotImplementedException();
        }

        [RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
        public static MethodDelegate CreateMethod(this MethodInfo methodInfo, bool isStatic)
        {
            var parameters = methodInfo.GetParameters();
            string delegateName;
            Type[] genericTypes;
            if (parameters.Length == 0)
            {
                if (methodInfo.ReturnType == typeof(void))
                {
                    delegateName = "CreateMethodDelegate0";
                    genericTypes = new[] { methodInfo.DeclaringType };
                }
                else
                {
                    delegateName = "CreateMethodDelegate1";
                    genericTypes = new[] { methodInfo.DeclaringType, methodInfo.ReturnType };
                }
            }
            else if (parameters.Length == 1)
            {
                if (methodInfo.ReturnType == typeof(void))
                {
                    delegateName = "CreateMethodDelegate3";
                    genericTypes = new[] { methodInfo.DeclaringType, parameters[0].ParameterType };
                }
                else
                {
                    delegateName = "CreateMethodDelegate4";
                    genericTypes = new[] { methodInfo.DeclaringType, parameters[0].ParameterType, methodInfo.ReturnType };
                }
            }
            else if (parameters.Length == 2)
            {
                // e.g. Dictionary.Add(key, value)
                if (methodInfo.ReturnType == typeof(void))
                {
                    delegateName = "CreateMethodDelegate5";
                    genericTypes = new[] { methodInfo.DeclaringType, parameters[0].ParameterType, parameters[1].ParameterType };
                }
                else
                {
                    throw new NotSupportedException($"{methodInfo.Name}: 2 parameters with return type.");
                }
            }
            else
            {
                throw new NotSupportedException($"{methodInfo.Name}: more than 2 parameters.");
            }

            MethodInfo m1 = typeof(ReflectionExtentions).GetMethod(delegateName, BindingFlags.Static | BindingFlags.NonPublic);
            MethodInfo m2 = m1.MakeGenericMethod(genericTypes);
            return (MethodDelegate)m2.Invoke(null, new object[] { methodInfo });
        }

        static Func<object, object> CreateFuncDelegate<TTarget, TReturn>(MethodInfo methodInfo)
        {
            if (methodInfo.DeclaringType.IsValueType)
            {
                var func = (FuncRef<TTarget, TReturn>)methodInfo.CreateDelegate(typeof(FuncRef<TTarget, TReturn>));
                return obj => { TTarget target = (TTarget)obj; return func(ref target); };
            }
            else
            {
                var func = (Func<TTarget, TReturn>)methodInfo.CreateDelegate(typeof(Func<TTarget, TReturn>));
                return obj => func((TTarget)obj);
            }
        }

        static Action<object, object> CreateActionDelegate<TTarget, TParam>(MethodInfo methodInfo)
        {
            var action = (Action<TTarget, TParam>)methodInfo.CreateDelegate(typeof(Action<TTarget, TParam>));
            return (obj, p1) => action((TTarget)obj, (TParam)p1);
        }

        // 0 parameter, No return type
        static MethodDelegate CreateMethodDelegate0<TTarget>(MethodInfo methodInfo)
        {
            var action = (Action<TTarget>)methodInfo.CreateDelegate(typeof(Action<TTarget>));
            return (obj, p) => { action((TTarget)obj); return null; };
        }

        // 0 parameter, with return type
        static MethodDelegate CreateMethodDelegate2<TTarget, TReturn>(MethodInfo methodInfo)
        {
            var func = (Func<TTarget, TReturn>)methodInfo.CreateDelegate(typeof(Func<TTarget, TReturn>));
            return (obj, p) => func((TTarget)obj);
        }

        // 1 parameter, No return type
        static MethodDelegate CreateMethodDelegate3<TTarget, TParam>(MethodInfo methodInfo)
        {
            var action = (Action<TTarget, TParam>)methodInfo.CreateDelegate(typeof(Action<TTarget, TParam>));
            return (obj, p) => { action((TTarget)obj, (TParam)p[0]); return null; };
        }

        // 1 parameter, with return type
        static MethodDelegate CreateMethodDelegate4<TTarget, TParam, TReturn>(MethodInfo methodInfo)
        {
            var func = (Func<TTarget, TParam, TReturn>)methodInfo.CreateDelegate(typeof(Func<TTarget, TParam, TReturn>));
            return (obj, p) => func((TTarget)obj, (TParam)p[0]);
        }

        // 2 parameters, No return type
        static MethodDelegate CreateMethodDelegate5<TTarget, TParam1, TParam2>(MethodInfo methodInfo)
        {
            var action = (Action<TTarget, TParam1, TParam2>)methodInfo.CreateDelegate(typeof(Action<TTarget, TParam1, TParam2>));
            return (obj, p) => { action((TTarget)obj, (TParam1)p[0], (TParam2)p[1]); return null; };
        }
    }
}
