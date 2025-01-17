// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/checker_image_tracker.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/test/skia_common.h"
#include "cc/tiles/image_controller.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

// 5MB max image cache size.
const size_t kMaxImageCacheSizeBytes = 5 * 1024 * 1024;

const int kCheckerableImageDimension = 512;
// This size will result in an image just over kMaxImageCacheSizeBytes.
const int kLargeNonCheckerableImageDimension = 1145;
const int kSmallNonCheckerableImageDimension = 16;

class TestImageController : public ImageController {
 public:
  // We can use the same thread for the image worker because all use of it in
  // the ImageController is over-ridden here.
  TestImageController()
      : ImageController(base::ThreadTaskRunnerHandle::Get().get(),
                        base::ThreadTaskRunnerHandle::Get()) {
    SetMaxImageCacheLimitBytesForTesting(kMaxImageCacheSizeBytes);
  }

  ~TestImageController() override { DCHECK_EQ(locked_images_.size(), 0U); }

  int num_of_locked_images() const { return locked_images_.size(); }
  const ImageIdFlatSet& decodes_requested() const { return decodes_requested_; }

  void UnlockImageDecode(ImageDecodeRequestId id) override {
    DCHECK_EQ(locked_images_.count(id), 1U);
    locked_images_.erase(id);
  }

  ImageDecodeRequestId QueueImageDecode(
      sk_sp<const SkImage> image,
      const ImageDecodedCallback& callback) override {
    ImageDecodeRequestId request_id = next_image_request_id_++;

    // The tracker should request a decode only once.
    EXPECT_EQ(decodes_requested_.count(image->uniqueID()), 0u);
    decodes_requested_.insert(image->uniqueID());

    locked_images_.insert(request_id);

    // Post the callback asynchronously to match the behaviour in
    // ImageController.
    worker_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(callback, request_id, ImageDecodeResult::SUCCESS));

    return request_id;
  }

 private:
  ImageDecodeRequestId next_image_request_id_ = 1U;
  std::unordered_set<ImageDecodeRequestId> locked_images_;
  ImageIdFlatSet decodes_requested_;
};

class CheckerImageTrackerTest : public testing::Test,
                                public CheckerImageTrackerClient {
 public:
  enum class ImageType {
    CHECKERABLE,
    SMALL_NON_CHECKERABLE,
    LARGE_NON_CHECKERABLE
  };

  void SetUpTracker(bool checker_images_enabled) {
    checker_image_tracker_ = base::MakeUnique<CheckerImageTracker>(
        &image_controller_, this, checker_images_enabled);
  }

  void TearDown() override { checker_image_tracker_.reset(); }

  DrawImage CreateImage(ImageType image_type) {
    int dimension = 0;
    switch (image_type) {
      case ImageType::CHECKERABLE:
        dimension = kCheckerableImageDimension;
        break;
      case ImageType::SMALL_NON_CHECKERABLE:
        dimension = kSmallNonCheckerableImageDimension;
        break;
      case ImageType::LARGE_NON_CHECKERABLE:
        dimension = kLargeNonCheckerableImageDimension;
        break;
    }

    sk_sp<SkImage> image =
        CreateDiscardableImage(gfx::Size(dimension, dimension));
    gfx::ColorSpace target_color_space = gfx::ColorSpace::CreateSRGB();
    return DrawImage(image, SkIRect::MakeWH(image->width(), image->height()),
                     kNone_SkFilterQuality, SkMatrix::I(), target_color_space);
  }

  CheckerImageTracker::ImageDecodeQueue BuildImageDecodeQueue(
      std::vector<DrawImage> draw_images,
      WhichTree tree) {
    CheckerImageTracker::ImageDecodeQueue decode_queue;
    for (auto draw_image : draw_images) {
      sk_sp<const SkImage> image = draw_image.image();
      if (checker_image_tracker_->ShouldCheckerImage(image, tree))
        decode_queue.push_back(image);
    }
    return decode_queue;
  }

  // CheckerImageTrackerClient implementation.
  void NeedsInvalidationForCheckerImagedTiles() override {
    invalidation_request_pending_ = true;
  }

 protected:
  TestImageController image_controller_;
  std::unique_ptr<CheckerImageTracker> checker_image_tracker_;

  bool invalidation_request_pending_ = false;
};

TEST_F(CheckerImageTrackerTest, CheckerImagesDisabled) {
  // Ensures that the tracker doesn't filter any images for checkering if it is
  // disabled.
  SetUpTracker(false);

  ImageIdFlatSet checkered_images;
  DrawImage draw_image = CreateImage(ImageType::CHECKERABLE);
  EXPECT_FALSE(checker_image_tracker_->ShouldCheckerImage(
      draw_image.image(), WhichTree::PENDING_TREE));
  EXPECT_EQ(image_controller_.num_of_locked_images(), 0);
}

TEST_F(CheckerImageTrackerTest, UpdatesImagesAtomically) {
  // Ensures that the tracker updates images atomically for each frame.
  SetUpTracker(true);

  DrawImage checkerable_image = CreateImage(ImageType::CHECKERABLE);
  DrawImage small_non_checkerable_image =
      CreateImage(ImageType::SMALL_NON_CHECKERABLE);
  DrawImage large_non_checkerable_image =
      CreateImage(ImageType::LARGE_NON_CHECKERABLE);
  CheckerImageTracker::ImageDecodeQueue image_decode_queue;

  // First request to filter images.
  std::vector<DrawImage> draw_images = {
      checkerable_image, small_non_checkerable_image,
      large_non_checkerable_image, checkerable_image};
  image_decode_queue =
      BuildImageDecodeQueue(draw_images, WhichTree::PENDING_TREE);

  ASSERT_EQ(2u, image_decode_queue.size());
  EXPECT_EQ(checkerable_image.image(), image_decode_queue[0]);
  EXPECT_EQ(checkerable_image.image(), image_decode_queue[1]);

  checker_image_tracker_->ScheduleImageDecodeQueue(image_decode_queue);
  EXPECT_EQ(image_controller_.num_of_locked_images(), 1);

  // Run pending task to indicate completion of decode request to the tracker.
  // This should send an impl-side invalidation request to the client. The
  // images must remain locked until the sync tree to which the invalidations
  // are added is activated.
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(invalidation_request_pending_);
  EXPECT_EQ(image_controller_.num_of_locked_images(), 1);

  // Continue checkering the image until the set of images to invalidate is
  // pulled.
  EXPECT_TRUE(checker_image_tracker_->ShouldCheckerImage(
      checkerable_image.image(), WhichTree::PENDING_TREE));

  ImageIdFlatSet invalidated_images =
      checker_image_tracker_->TakeImagesToInvalidateOnSyncTree();
  EXPECT_EQ(invalidated_images.size(), 1U);
  EXPECT_EQ(invalidated_images.count(checkerable_image.image()->uniqueID()),
            1U);

  // Use the same set of draw images to ensure that they are not checkered on
  // the pending tree now.
  EXPECT_FALSE(checker_image_tracker_->ShouldCheckerImage(
      checkerable_image.image(), WhichTree::PENDING_TREE));
  EXPECT_FALSE(checker_image_tracker_->ShouldCheckerImage(
      small_non_checkerable_image.image(), WhichTree::PENDING_TREE));
  EXPECT_FALSE(checker_image_tracker_->ShouldCheckerImage(
      large_non_checkerable_image.image(), WhichTree::PENDING_TREE));

  // Use this set to make the same request from the active tree, we should
  // continue checkering this image on the active tree until activation.
  EXPECT_TRUE(checker_image_tracker_->ShouldCheckerImage(
      checkerable_image.image(), WhichTree::ACTIVE_TREE));
  EXPECT_FALSE(checker_image_tracker_->ShouldCheckerImage(
      small_non_checkerable_image.image(), WhichTree::ACTIVE_TREE));
  EXPECT_FALSE(checker_image_tracker_->ShouldCheckerImage(
      large_non_checkerable_image.image(), WhichTree::ACTIVE_TREE));

  // Activate the sync tree. The images should be unlocked upon activation.
  EXPECT_EQ(image_controller_.num_of_locked_images(), 1);
  checker_image_tracker_->DidActivateSyncTree();
}

TEST_F(CheckerImageTrackerTest, NoConsecutiveCheckeringForImage) {
  // Ensures that if an image is decoded and invalidated once, it is not
  // checkered again in subsequent frames.
  SetUpTracker(true);

  DrawImage checkerable_image = CreateImage(ImageType::CHECKERABLE);
  std::vector<DrawImage> draw_images = {checkerable_image};

  CheckerImageTracker::ImageDecodeQueue image_decode_queue =
      BuildImageDecodeQueue(draw_images, WhichTree::PENDING_TREE);
  EXPECT_EQ(image_decode_queue.size(), 1U);
  checker_image_tracker_->ScheduleImageDecodeQueue(image_decode_queue);

  // Trigger decode completion, take images to invalidate and activate the sync
  // tree.
  base::RunLoop().RunUntilIdle();
  checker_image_tracker_->TakeImagesToInvalidateOnSyncTree();
  checker_image_tracker_->DidActivateSyncTree();

  // Subsequent requests for this image should not be checkered.
  EXPECT_FALSE(checker_image_tracker_->ShouldCheckerImage(
      checkerable_image.image(), WhichTree::PENDING_TREE));
}

TEST_F(CheckerImageTrackerTest,
       TracksCheckeredImagesSeperatelyInConsecutiveFrames) {
  // Ensures that the set of images being checkered on the pending tree, and the
  // active tree are tracked correctly.
  SetUpTracker(true);

  DrawImage checkerable_image1 = CreateImage(ImageType::CHECKERABLE);
  std::vector<DrawImage> draw_images;
  CheckerImageTracker::ImageDecodeQueue image_decode_queue;

  // First request to filter images on the pending and active tree.
  draw_images.push_back(checkerable_image1);
  image_decode_queue =
      BuildImageDecodeQueue(draw_images, WhichTree::PENDING_TREE);
  EXPECT_EQ(image_decode_queue.size(), 1U);
  checker_image_tracker_->ScheduleImageDecodeQueue(image_decode_queue);

  // The image is also checkered on the active tree while a decode request is
  // pending.
  EXPECT_TRUE(checker_image_tracker_->ShouldCheckerImage(
      checkerable_image1.image(), WhichTree::ACTIVE_TREE));

  // Trigger decode completion and take images to invalidate on the sync tree.
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(invalidation_request_pending_);
  ImageIdFlatSet invalidated_images =
      checker_image_tracker_->TakeImagesToInvalidateOnSyncTree();
  EXPECT_EQ(invalidated_images.size(), 1U);
  EXPECT_EQ(invalidated_images.count(checkerable_image1.image()->uniqueID()),
            1U);

  // Second request to filter the same image on the pending and active tree. It
  // should be checkered on the active tree, but not the pending tree.
  EXPECT_TRUE(checker_image_tracker_->ShouldCheckerImage(
      checkerable_image1.image(), WhichTree::ACTIVE_TREE));
  EXPECT_FALSE(checker_image_tracker_->ShouldCheckerImage(
      checkerable_image1.image(), WhichTree::PENDING_TREE));

  // New checkerable image on the pending tree.
  DrawImage checkerable_image2 = CreateImage(ImageType::CHECKERABLE);
  EXPECT_TRUE(checker_image_tracker_->ShouldCheckerImage(
      checkerable_image2.image(), WhichTree::PENDING_TREE));

  // Activate the sync tree. The initial image should no longer be checkered on
  // the active tree.
  checker_image_tracker_->DidActivateSyncTree();
  EXPECT_FALSE(checker_image_tracker_->ShouldCheckerImage(
      checkerable_image1.image(), WhichTree::ACTIVE_TREE));
}

TEST_F(CheckerImageTrackerTest, CancelsScheduledDecodes) {
  SetUpTracker(true);

  DrawImage checkerable_image1 = CreateImage(ImageType::CHECKERABLE);
  DrawImage checkerable_image2 = CreateImage(ImageType::CHECKERABLE);
  std::vector<DrawImage> draw_images = {checkerable_image1, checkerable_image2};

  CheckerImageTracker::ImageDecodeQueue image_decode_queue;
  image_decode_queue =
      BuildImageDecodeQueue(draw_images, WhichTree::PENDING_TREE);
  EXPECT_EQ(image_decode_queue.size(), 2U);
  checker_image_tracker_->ScheduleImageDecodeQueue(
      std::move(image_decode_queue));

  // Only the first image in the queue should have been decoded.
  EXPECT_EQ(image_controller_.decodes_requested().size(), 1U);
  EXPECT_EQ(image_controller_.decodes_requested().count(
                checkerable_image1.image()->uniqueID()),
            1U);

  // Rebuild the queue before the tracker is notified of decode completion,
  // removing the second image and adding a new one.
  DrawImage checkerable_image3 = CreateImage(ImageType::CHECKERABLE);
  draw_images = {checkerable_image1, checkerable_image3};
  image_decode_queue =
      BuildImageDecodeQueue(draw_images, WhichTree::PENDING_TREE);

  // The queue has 2 decodes because we are still checkering on the first one.
  EXPECT_EQ(image_decode_queue.size(), 2U);
  checker_image_tracker_->ScheduleImageDecodeQueue(
      std::move(image_decode_queue));

  // We still have only one decode because the tracker keeps only one decode
  // pending at a time.
  EXPECT_EQ(image_controller_.decodes_requested().size(), 1U);
  EXPECT_EQ(image_controller_.decodes_requested().count(
                checkerable_image1.image()->uniqueID()),
            1U);

  // Trigger completion for all decodes. Only 2 images should have been decoded
  // since the second image was cancelled.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(image_controller_.decodes_requested().size(), 2U);
  EXPECT_EQ(image_controller_.decodes_requested().count(
                checkerable_image3.image()->uniqueID()),
            1U);
  EXPECT_EQ(image_controller_.num_of_locked_images(), 2);
}

}  // namespace
}  // namespace cc
