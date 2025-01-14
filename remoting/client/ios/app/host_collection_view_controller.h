// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_IOS_APP_HOST_COLLECTION_VIEW_CONTOLLER_H_
#define REMOTING_CLIENT_IOS_APP_HOST_COLLECTION_VIEW_CONTOLLER_H_

#import <UIKit/UIKit.h>

#import "ios/third_party/material_components_ios/src/components/Collections/src/MaterialCollections.h"
#import "ios/third_party/material_components_ios/src/components/FlexibleHeader/src/MaterialFlexibleHeader.h"
#import "remoting/client/ios/app/host_collection_view_cell.h"
#import "remoting/client/ios/domain/host_info.h"

// The host collection view controller delegate provides the data for available
// hosts and receives selection events from the collection view controller.
@protocol HostCollectionViewControllerDelegate<NSObject>

// Notifies the delegate if a selection happens for the provided cell.
// The delegate should run the completionBlock when processing for this event
// has finished.
@optional
- (void)didSelectCell:(HostCollectionViewCell*)cell
           completion:(void (^)())completionBlock;

// The delegate should provide the HostInfo object for the given path if
// available from the cache.
- (HostInfo*)getHostAtIndexPath:(NSIndexPath*)path;

// The delegate must provide the total number of hosts currently cached.
- (NSInteger)getHostCount;

@end

@interface HostCollectionViewController : MDCCollectionViewController

@property(weak, nonatomic) id<HostCollectionViewControllerDelegate> delegate;
@property(nonatomic)
    MDCFlexibleHeaderContainerViewController* flexHeaderContainerViewController;

@end

#endif  // REMOTING_CLIENT_IOS_APP_HOST_LIST_VIEW_CONTOLLER_H_
