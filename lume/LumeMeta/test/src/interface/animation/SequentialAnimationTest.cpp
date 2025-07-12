/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/animation.h>
#include <meta/interface/property/construct_property.h>

#include "TestRunner.h"
#include "helpers/animation_test_base.h"
#include "helpers/testing_objects.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class SequentialAnimationTest : public AnimationTestBase {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
};

/**
 * @tc.name: SeekingSequentialAnimation
 * @tc.desc: test SeekingSequentialAnimation
 * @tc.type: FUNC
 */
HWTEST_F(SequentialAnimationTest, SeekingSequentialAnimationTest, TestSize.Level1)
{
    // given
    auto property1 = ConstructProperty("Property1", 0.0F);
    auto property2 = ConstructProperty("Property2", 0.0F);

    auto animation = SequentialAnimation(CreateInstance(ClassId::SequentialAnimation))
                         .Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation))
                                  .SetFrom(0.0F)
                                  .SetTo(1.0F)
                                  .SetProperty(property1)
                                  .SetDuration(TimeSpan::Milliseconds(1000)))
                         .Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation))
                                  .SetFrom(0.0F)
                                  .SetTo(1.0F)
                                  .SetProperty(property2)
                                  .SetDuration(TimeSpan::Milliseconds(1000)));
    // when
    animation.Seek(0.25F);
    animation.Start();
    animation.Pause();
    auto progress25Property1Value = property1->GetValue();
    auto progress25Property2Value = property2->GetValue();

    animation.Seek(0.5F);
    animation.Start();
    animation.Pause();

    auto progress50Property1Value = property1->GetValue();
    auto progress50Property2Value = property2->GetValue();

    animation.Seek(0.75F);
    animation.Start();
    animation.Pause();

    auto progress75Property1Value = property1->GetValue();
    auto progress75Property2Value = property2->GetValue();

    // expected
    EXPECT_THAT(progress25Property1Value, testing::Eq(0.5F));
    EXPECT_THAT(progress25Property2Value, testing::Eq(0.0F));

    EXPECT_THAT(progress50Property1Value, testing::Eq(1.0F));
    EXPECT_THAT(progress50Property2Value, testing::Eq(0.0F));

    EXPECT_THAT(progress75Property1Value, testing::Eq(1.0F));
    EXPECT_THAT(progress75Property2Value, testing::Eq(0.5F));
}

/**
 * @tc.name: SeekingSequentialAnimationAndPause
 * @tc.desc: test SeekingSequentialAnimationAndPause
 * @tc.type: FUNC
 */
HWTEST_F(SequentialAnimationTest, SeekingSequentialAnimationAndPause, TestSize.Level1)
{
    // given
    auto property1 = ConstructProperty("Property1", 0.0F);
    auto property2 = ConstructProperty("Property2", 0.0F);

    auto animation = SequentialAnimation(CreateInstance(ClassId::SequentialAnimation))
                         .Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation))
                                  .SetFrom(0.0F)
                                  .SetTo(1.0F)
                                  .SetProperty(property1)
                                  .SetDuration(TimeSpan::Milliseconds(1000)))
                         .Add(KeyframeAnimation<float>(CreateInstance(ClassId::KeyframeAnimation))
                                  .SetFrom(0.0F)
                                  .SetTo(1.0F)
                                  .SetProperty(property2)
                                  .SetDuration(TimeSpan::Milliseconds(1000)));

    // when
    animation.Start();
    animation.Seek(0.25F);
    animation.Pause();

    auto progress25Property1Value = property1->GetValue();
    auto progress25Property2Value = property2->GetValue();

    animation.Start();
    animation.Seek(0.5F);
    animation.Pause();

    auto progress50Property1Value = property1->GetValue();
    auto progress50Property2Value = property2->GetValue();

    animation.Start();
    animation.Seek(0.75F);
    animation.Pause();

    auto progress75Property1Value = property1->GetValue();
    auto progress75Property2Value = property2->GetValue();

    // expected
    EXPECT_THAT(progress25Property1Value, testing::Eq(0.5F));
    EXPECT_THAT(progress25Property2Value, testing::Eq(0.0F));

    EXPECT_THAT(progress50Property1Value, testing::Eq(1.0F));
    EXPECT_THAT(progress50Property2Value, testing::Eq(0.0F));

    EXPECT_THAT(progress75Property1Value, testing::Eq(1.0F));
    EXPECT_THAT(progress75Property2Value, testing::Eq(0.5F));
}

META_END_NAMESPACE()
