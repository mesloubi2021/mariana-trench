/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.marianatrench.integrationtests

/**
 * Test cases covering flow of taint through anonymous classes in Kotlin, typically achieved through
 * lambdas. Coroutines are another common source of anonymous classes, but the coroutine libraries
 * are not mocked in the integration tests.
 */
public class KotlinAnonymousFunction {
  fun issue() {
    var source: Object? = null

    val lambda = { -> source = Origin.source() as Object }
    lambda()

    Origin.sink(source)
  }
}
