// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Serialization
{
#if NET45 || NETSTANDARD1_3 || MONOANDROID
    using System;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Runtime.Serialization;

    static class ReflectionExtentions
    {
        static readonly Type[] delegateParamsType = { typeof(object), typeof(object[]) };

        public static Func<object, object> CreateGetter(this FieldInfo fieldInfo)
        {
            DynamicMethod method = new DynamicMethod(GetAccessorName(true, fieldInfo.Name), typeof(object), new[] { typeof(object) }, true);
            ILGenerator generator = method.GetILGenerator();
            generator.Emit(OpCodes.Ldarg_0);
            EmitTypeConversion(generator, fieldInfo.DeclaringType, true);
            generator.Emit(OpCodes.Ldfld, fieldInfo);
            if (fieldInfo.FieldType.GetTypeInfo().IsValueType)
            {
                generator.Emit(OpCodes.Box, fieldInfo.FieldType);
            }

            generator.Emit(OpCodes.Ret);

            return (Func<object, object>)method.CreateDelegate(typeof(Func<object, object>));
        }

        public static Action<object, object> CreateSetter(this FieldInfo fieldInfo)
        {
            DynamicMethod method = new DynamicMethod(GetAccessorName(false, fieldInfo.Name), typeof(void), new[] { typeof(object), typeof(object) }, true);
            ILGenerator generator = method.GetILGenerator();
            generator.Emit(OpCodes.Ldarg_0);
            EmitTypeConversion(generator, fieldInfo.DeclaringType, true);
            generator.Emit(OpCodes.Ldarg_1);
            EmitTypeConversion(generator, fieldInfo.FieldType, false);
            generator.Emit(OpCodes.Stfld, fieldInfo);
            generator.Emit(OpCodes.Ret);

            return (Action<object, object>)method.CreateDelegate(typeof(Action<object, object>));
        }

        public static Func<object, object> CreateGetter(this PropertyInfo propertyInfo)
        {
            DynamicMethod method = new DynamicMethod(GetAccessorName(true, propertyInfo.Name), typeof(object), new[] { typeof(object) }, true);
            ILGenerator generator = method.GetILGenerator();
            generator.DeclareLocal(typeof(object));
            generator.Emit(OpCodes.Ldarg_0);
            EmitTypeConversion(generator, propertyInfo.DeclaringType, true);
            EmitCall(generator, propertyInfo.GetGetMethod(true));
            if (propertyInfo.PropertyType.GetTypeInfo().IsValueType)
            {
                generator.Emit(OpCodes.Box, propertyInfo.PropertyType);
            }

            generator.Emit(OpCodes.Ret);

            return (Func<object, object>)method.CreateDelegate(typeof(Func<object, object>));
        }

        public static Action<object, object> CreateSetter(this PropertyInfo propertyInfo, bool requiresSetter)
        {
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

            DynamicMethod method = new DynamicMethod(GetAccessorName(false, propertyInfo.Name), typeof(void), new[] { typeof(object), typeof(object) }, true);
            ILGenerator generator = method.GetILGenerator();
            generator.Emit(OpCodes.Ldarg_0);
            EmitTypeConversion(generator, propertyInfo.DeclaringType, true);
            generator.Emit(OpCodes.Ldarg_1);
            EmitTypeConversion(generator, propertyInfo.PropertyType, false);
            EmitCall(generator, setMethod);
            generator.Emit(OpCodes.Ret);

            return (Action<object, object>)method.CreateDelegate(typeof(Action<object, object>));
        }

        public static MethodDelegate CreateMethod(this ConstructorInfo constructorInfo)
        {
            DynamicMethod method = new DynamicMethod("ctor_" + constructorInfo.DeclaringType.Name, typeof(object), delegateParamsType, true);
            Type[] paramsType = GetParametersType(constructorInfo.GetParameters());
            ILGenerator generator = method.GetILGenerator();
            LoadArguments(generator, paramsType);
            generator.Emit(OpCodes.Newobj, constructorInfo);
            if (constructorInfo.DeclaringType.GetTypeInfo().IsValueType)
            {
                generator.Emit(OpCodes.Box, constructorInfo.DeclaringType);
            }

            generator.Emit(OpCodes.Ret);

            return (MethodDelegate)method.CreateDelegate(typeof(MethodDelegate));
        }

        public static MethodDelegate CreateMethod(this MethodInfo methodInfo, bool isStatic)
        {
            Type[] paramsType = GetParametersType(methodInfo.GetParameters());
            DynamicMethod method = new DynamicMethod("method_" + methodInfo.Name, typeof(object), delegateParamsType, true);
            ILGenerator generator = method.GetILGenerator();
            if (!isStatic)
            {
                generator.Emit(OpCodes.Ldarg_0);
                if (methodInfo.DeclaringType.GetTypeInfo().IsValueType)
                {
                    generator.Emit(OpCodes.Unbox_Any, methodInfo.DeclaringType);
                }
                else
                {
                    generator.Emit(OpCodes.Castclass, methodInfo.DeclaringType);
                }
            }
            
            LoadArguments(generator, paramsType);
            if (methodInfo.IsFinal)
            {
                generator.Emit(OpCodes.Call, methodInfo);
            }
            else
            {
                generator.Emit(OpCodes.Callvirt, methodInfo);
            }

            if (methodInfo.ReturnType == typeof(void))
            {
                generator.Emit(OpCodes.Ldnull);
            }
            else if (methodInfo.ReturnType.GetTypeInfo().IsValueType)
            {
                generator.Emit(OpCodes.Box, methodInfo.ReturnType);
            }

            generator.Emit(OpCodes.Ret);

            return (MethodDelegate)method.CreateDelegate(typeof(MethodDelegate));
        }

        static void EmitTypeConversion(ILGenerator generator, Type castType, bool isContainer)
        {
            if (castType == typeof(object))
            {
            }
            else if (castType.GetTypeInfo().IsValueType)
            {
                generator.Emit(isContainer ? OpCodes.Unbox : OpCodes.Unbox_Any, castType);
            }
            else
            {
                generator.Emit(OpCodes.Castclass, castType);
            }
        }

        static void EmitCall(ILGenerator generator, MethodInfo method)
        {
            OpCode opcode = (method.IsStatic || method.DeclaringType.GetTypeInfo().IsValueType) ? OpCodes.Call : OpCodes.Callvirt;
            generator.EmitCall(opcode, method, null);
        }

        static string GetAccessorName(bool isGetter, string name)
        {
            return (isGetter ? "get_" : "set_") + name;
        }

        static Type[] GetParametersType(ParameterInfo[] paramsInfo)
        {
            Type[] paramsType = new Type[paramsInfo.Length];
            for (int i = 0; i < paramsInfo.Length; i++)
            {
                paramsType[i] = paramsInfo[i].ParameterType.IsByRef ? paramsInfo[i].ParameterType.GetElementType() : paramsInfo[i].ParameterType;
            }

            return paramsType;
        }

        static void LoadArguments(ILGenerator generator, Type[] paramsType)
        {
            for (int i = 0; i < paramsType.Length; i++)
            {
                generator.Emit(OpCodes.Ldarg_1);
                switch (i)
                {
                    case 0: generator.Emit(OpCodes.Ldc_I4_0); break;
                    case 1: generator.Emit(OpCodes.Ldc_I4_1); break;
                    case 2: generator.Emit(OpCodes.Ldc_I4_2); break;
                    case 3: generator.Emit(OpCodes.Ldc_I4_3); break;
                    case 4: generator.Emit(OpCodes.Ldc_I4_4); break;
                    case 5: generator.Emit(OpCodes.Ldc_I4_5); break;
                    case 6: generator.Emit(OpCodes.Ldc_I4_6); break;
                    case 7: generator.Emit(OpCodes.Ldc_I4_7); break;
                    case 8: generator.Emit(OpCodes.Ldc_I4_8); break;
                    default: generator.Emit(OpCodes.Ldc_I4, i); break;
                }
                generator.Emit(OpCodes.Ldelem_Ref);
                if (paramsType[i].GetTypeInfo().IsValueType)
                {
                    generator.Emit(OpCodes.Unbox_Any, paramsType[i]);
                }
                else if (paramsType[i] != typeof(object))
                {
                    generator.Emit(OpCodes.Castclass, paramsType[i]);
                }
            }
        }
    }
#endif
#if NETSTANDARD2_0 || NET8_0_OR_GREATER
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

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
#endif
        public static Func<object, object> CreateGetter(this PropertyInfo propertyInfo)
        {
            MethodInfo getMethod = propertyInfo.GetGetMethod(true);
            MethodInfo m1 = typeof(ReflectionExtentions).GetMethod("CreateFuncDelegate", BindingFlags.Static | BindingFlags.NonPublic);
            MethodInfo m2 = m1.MakeGenericMethod(propertyInfo.DeclaringType, propertyInfo.PropertyType);
            return (Func<object, object>)m2.Invoke(null, new object[] { getMethod });
        }

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
#endif
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

#if NET8_0_OR_GREATER
        [System.Diagnostics.CodeAnalysis.RequiresDynamicCode(AmqpContractSerializer.AotWarning)]
#endif
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
#endif
}
