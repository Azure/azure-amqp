// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Reflection;

    internal static class ExceptionExtensions
    {
        const string ExceptionIdentifierName = "Microsoft.Azure.Amqp.ExceptionId";

        public static string ToStringSlim(this Exception exception)
        {
            // exception.Data is empty collection by default.
            if (exception.Data != null && exception.Data.Contains(ExceptionIdentifierName))
            {
                return string.Format(CultureInfo.InvariantCulture,
                    "ExceptionId: {0}-{1}: {2}",
                    exception.Data[ExceptionIdentifierName],
                    exception.GetType(),
                    exception.Message);
            }
            else if (exception.Data != null)
            {
                string exceptionIdentifier = Guid.NewGuid().ToString();
                exception.Data[ExceptionIdentifierName] = exceptionIdentifier;

                return string.Format(CultureInfo.InvariantCulture,
                    "ExceptionId: {0}-{1}",
                    exceptionIdentifier,
                    exception.ToString());
            }

            // In case Data collection in the exception is nullified.
            return exception.ToString();
        }
    }
}
