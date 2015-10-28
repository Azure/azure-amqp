// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

namespace Microsoft.Azure.Amqp.Tracing
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Xml;

    /// <summary>
    /// TrackingContext class is used to communicate tracking information for each request.
    /// </summary>
    public sealed class TrackingContext
    {
        internal const string DeviceGatewaySubsystemString = "DeviceGateway";
        internal const string SlimQueueSubsystemString = "SlimQueue";
        internal const string NoTrackingId = "NoTrackingId";
        internal const string NoSystemTracker = "NoSystemTracker";
        public const string TrackingIdName = "TrackingId";
        public const string SystemTrackerName = "SystemTracker";
        public const string HeaderNamespace = "http://schemas.microsoft.com/servicebus/2010/08/protocol/";
        const string AppendRoleSeperator = "_";
        const string MessageIdHeaderName = "MessageId";

        static char AppendRolePrefix;

        static readonly Dictionary<string, Guid> ComponentTrackingGuids = new Dictionary<string, Guid>()
            {
                { DeviceGatewaySubsystemString, new Guid("{32E01E6F-B1B1-444C-8A29-F40D6DD1EDB7}") },
                { SlimQueueSubsystemString, new Guid("{20204884-C8E1-4C8B-8865-E1ABB0587D85}") },
            };

        readonly string trackingId;
        readonly string systemTracker;
        EventTraceActivity eventTraceActivity;

        // For use only by factory.
        TrackingContext(string trackingId, string systemTracker)
        {
            this.trackingId = trackingId;
            this.systemTracker = systemTracker;
        }

        // RoleId is set 
        //      - by the MessagingHostComponent that loads the broker in case of broker role
        //      - by the NetMessagingProtocolHeadService in case of gateway role
        internal static string RoleIdentifier { get; private set; }

        /// <summary> Returns the tracking id used in this context </summary>
        internal string TrackingId
        {
            get { return (string.IsNullOrEmpty(this.trackingId)) ? TrackingContext.NoTrackingId : this.trackingId; }
        }

        /// <summary> Returns the system tracker used in this context </summary>
        internal string SystemTracker
        {
            get { return (string.IsNullOrEmpty(this.systemTracker)) ? TrackingContext.NoSystemTracker : this.systemTracker; }
        }

        /// <summary>
        /// Return a tracking context for a key that is common for a subsystem. It generates a new Guid if the key is not present in the collection.
        /// </summary>
        /// <param name="key">key to lookup an existing Guid for TrackingId</param>
        /// <param name="overrideSystemTracker">subsystem-specific string like entityName to be used in the tracking context</param>
        /// <returns>TrackingContext</returns>
        internal static TrackingContext GetInstanceFromKey(string key, string overrideSystemTracker)
        {
            Guid trackingGuid;
            string systemTracker;
            if (!TrackingContext.ComponentTrackingGuids.TryGetValue(key, out trackingGuid))
            {
                trackingGuid = Guid.NewGuid();
            }

            if (string.IsNullOrEmpty((overrideSystemTracker)))
            {
                systemTracker = string.Empty;
            }
            else
            {
                systemTracker = overrideSystemTracker;
            }

            return GetInstance(trackingGuid, systemTracker);
        }

        // Used to generate TrackingContext given a key and when the system tracker is not known.
        internal static TrackingContext GetInstanceFromKey(string key)
        {
            Guid trackingGuid;
            if (!TrackingContext.ComponentTrackingGuids.TryGetValue(key, out trackingGuid))
            {
                trackingGuid = Guid.NewGuid();
            }

            return GetInstance(trackingGuid, null);
        }

        /// <summary>
        /// Return a tracking context for the given trackingId.
        /// </summary>
        /// <param name="guidTrackingId">TrackingId to use for the tracking context</param>
        /// <param name="overrideSystemTracker">subsystem-specific string like entityName to be used in the tracking context</param>
        /// <returns>TrackingContext</returns>
        internal static TrackingContext GetInstance(Guid guidTrackingId, string overrideSystemTracker)
        {
            string trackingId;
            string systemTracker;
            if (string.IsNullOrEmpty((overrideSystemTracker)))
            {
                systemTracker = string.Empty;
            }
            else
            {
                systemTracker = overrideSystemTracker;
            }

            trackingId = AppendRoleInstanceInformationToTrackingId(guidTrackingId.ToString());

            return new TrackingContext(trackingId, systemTracker);
        }

        // Used to generate TrackingContext given a tracking id and when the system tracker is not known.
        internal static TrackingContext GetInstance(Guid guidTrackingId)
        {
            return GetInstance(guidTrackingId, null);
        }

        /// <summary>
        /// Return a tracking context for the given trackingId.
        /// </summary>
        /// <param name="stringTrackingId">TrackingId to use for the tracking context</param>
        /// <param name="overrideSystemTracker">subsystem-specific string like entityName to be used in the tracking context</param>
        /// <param name="embedRoleInformation">Generally false. Set to true only when the message is received at role boundaries</param>
        /// <returns>TrackingContext</returns>
        internal static TrackingContext GetInstance(string stringTrackingId, string overrideSystemTracker, bool embedRoleInformation)
        {
            string trackingId;
            string systemTracker;
            if (string.IsNullOrEmpty((overrideSystemTracker)))
            {
                systemTracker = string.Empty;
            }
            else
            {
                systemTracker = overrideSystemTracker;
            }

            trackingId = stringTrackingId;
            if (embedRoleInformation)
            {
                trackingId = AppendRoleInstanceInformationToTrackingId(stringTrackingId);
            }

            return new TrackingContext(trackingId, systemTracker);
        }

        // Used to generate TrackingContext given a tracking id and when the system tracker is not known.
        internal static TrackingContext GetInstance(string stringTrackingId, bool embedRoleInformation)
        {
            return GetInstance(stringTrackingId, null, embedRoleInformation);
        }
        
        /// <summary>
        /// Sets the role identifier for the process' app domain. The role identifier field is singleton.
        /// It is called externally outside TrackingContext to avoid dependancy on Microsoft.WindowsAzure.ServiceRuntime
        /// </summary>
        /// <param name="roleIdentifier">the Role id</param>
        /// <param name="rolePrefix">Prefix for the role component</param>
        internal static void SetTrackingContextRoleIdentifier(string roleIdentifier, RolePrefix rolePrefix)
        {
            // The role identifier field is not set if it already has some value.
            if (string.IsNullOrEmpty(RoleIdentifier))
            {
                RoleIdentifier = roleIdentifier;
                AppendRolePrefix = (char)rolePrefix;
            }
        }

        internal string CreateClientTrackingExceptionInfo()
        {
            DateTime currentTime = DateTime.UtcNow;
            string clientTracingInfo = string.Format(CultureInfo.CurrentCulture, "TrackingId:{0},TimeStamp:{1}", this.TrackingId, currentTime);
            return clientTracingInfo;
        }

        static Guid GetTrackingId(UniqueId uniqueId)
        {
            Guid requestId;
            if (!uniqueId.TryGetGuid(out requestId))
            {
                return Guid.Empty;
            }

            return requestId;
        }

        internal EventTraceActivity Activity
        {
            get
            {
                if (eventTraceActivity == null)
                {
                    EventTraceActivity activity = GetActivity(this.trackingId);
                    this.eventTraceActivity = activity;
                }

                return this.eventTraceActivity;
            }
        }

        internal static EventTraceActivity GetActivity(string id)
        {
            EventTraceActivity activity = null;
            if (!string.IsNullOrEmpty(id))
            {
                int pos = id.IndexOf('_');
                Guid guidPart;
                if ((pos > 0 && Guid.TryParse(id.Substring(0, pos), out guidPart))
                    || Guid.TryParse(id, out guidPart))
                {
                    activity = new EventTraceActivity(guidPart);
                }
            }

            return activity ?? new EventTraceActivity();
        }

        internal static string AppendRoleInstanceInformationToTrackingId(string trackingId)
        {
            if (string.IsNullOrEmpty(RoleIdentifier))
            {
                return trackingId;
            }

            return trackingId + AppendRoleSeperator + AppendRolePrefix + RoleIdentifier;
        }

        internal static string GetRoleInstanceInformation()
        {
            if (string.IsNullOrEmpty(RoleIdentifier))
            {
                return string.Empty;
            }

            return AppendRoleSeperator + AppendRolePrefix + RoleIdentifier;
        }

        internal enum RolePrefix
        {
            Admin = 'A',
            Broker = 'B',
            Gateway = 'G',
            GeoMaster = 'M',
            Push = 'P',
            RPGateway = 'R',
        }
    }
}
