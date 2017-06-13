// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
// This file is NOT generated
//

namespace Microsoft.Azure.Amqp
{
    using System;
    using Windows.ApplicationModel.Resources.Core;

    class ResourceManagerImpl
    {
        private string resourceName;
        private readonly ResourceMap stringResourceMap;
        private readonly ResourceContext resourceContext;

        public ResourceManagerImpl(string resourceName)
        {
            this.resourceName = resourceName;
            this.stringResourceMap = ResourceManager.Current.MainResourceMap.GetSubtree("Microsoft.Azure.Amqp/" + resourceName);
            this.resourceContext = ResourceContext.GetForViewIndependentUse();
        }

        public string GetString(string name, System.Globalization.CultureInfo culture)
        {
            var value = this.stringResourceMap.GetValue(name, this.resourceContext).ValueAsString;
            if (string.IsNullOrEmpty(value))
            {
                throw new NotImplementedException($"Resource string '{name}' not found. Make sure it is added to '{this.resourceName}.resw'");
            }
            return value;
        }
    }
}
