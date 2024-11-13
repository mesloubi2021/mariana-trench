/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.marianatrench.integrationtests;

import java.util.Random;

class Data {
  Object foo;
  Object bar;
  Object baz;
}

public class TaintTransforms {
  static Object getNewSource() {
    return new Object();
  }

  static Object propagate(Object o) {
    return o;
  }

  static Object transformT1(Object o) {
    // Declared propagation: Argument(0) -> `T1@LocalReturn`
    // Expect inferred propagation: Argument(0) -> `LocalReturn`
    return o;
  }

  static Object transformT2(Object o) {
    // Declared frozen propagation: Argument(0) -> `T2@LocalReturn`
    return o;
  }

  static Object transformT1T2(Object o) {
    // Declared frozen propagation: Argument(0) -> `T1:T2@LocalReturn`
    return o;
  }

  static Object transformT3(Object o) {
    // Declared frozen propagation: Argument(0) -> `T3@LocalReturn`
    return o;
  }

  static Data transformT3WithSource(Data d) {
    // Declared frozen propagation: Argument(0) -> `T3@LocalReturn`
    // Expect inferred generation: Return.baz -> `NewSource`
    d.baz = getNewSource();
    return d;
  }

  public static Object propagateWithTransformT2T3(Object o) {
    // Expect inferred propagations:
    //   Argument(0) -> `T2@T3:LocalReturn`
    //               -> `LocalReturn`
    Object o1 = transformT2(o);
    Object o2 = transformT3(o1);
    return o2;
  }

  static Object transformUnused(Object o) {
    // Declared propagation: Argument(0) -> `Unused@LocalReturn`
    // Expect inferred propagation: Argument(0) -> `LocalReturn`
    return o;
  }

  static Object transformForAllParameters(Data d, Object o) {
    // Declared frozen propagation using for_all_parameters:
    //   Argument(1) -> `T1@LocalReturn`
    return o;
  }

  static Object propagateWithTransformT1(Object o) {
    // Expect inferred propagations:
    //   Argument(0) -> `T1:LocalReturn`
    //               -> `LocalReturn`
    return transformT1(o);
  }

  static Data propagateAlternateTransforms(Data d) {
    // Expect inferred propagations:
    //   Argument(0).foo -> `T1:LocalReturn`.baz
    //                   -> `LocalReturn`.baz
    //   Argument(0).bar -> `T2:LocalReturn`.foo
    //   Argument(0).baz -> `LocalReturn`.bar
    int rand = new Random().nextInt(2);
    Data result = new Data();

    if (rand == 0) {
      result.baz = transformT1(d.foo);
    } else if (rand == 1) {
      result.foo = transformT2(d.bar);
    } else {
      result.bar = d.baz;
    }

    return result;
  }

  static Data propagateWithTransformT3WithSource(Data d) {
    // Expect inferred propagation: Argument(0) -> `T3:LocalReturn`
    // Expect inferred generation: Return.baz -> `NewSource`
    return transformT3WithSource(d);
  }

  static Object propagateWithTransformsT1Unused(Object o) {
    // Expect inferred propagations:
    //   Argument(0) -> `T1:LocalReturn`
    //               -> `LocalReturn`
    Object t1 = transformT1(o);
    return transformUnused(t1);
  }

  static Object sourceWithTransformT1() {
    // Expect inferred generations:
    //   Return -> `T1@Source`
    //          -> `Source`
    Object source = Origin.source();
    Object sourceWithT1 = transformT1(source);
    return sourceWithT1;
  }

  static Object sourceWithTransformsT1T2() {
    // Expect inferred generations:
    //   Return -> `T2@T1:Source`
    //          -> `T2@Source`
    Object sourceT1 = transformT1(Origin.source());
    return transformT2(sourceT1);
  }

  static Object hopSourceWithTransformsT1T2() {
    // Expect inferred generations:
    //   Return -> `T2:T1@Source`
    //          -> `T2@Source`
    Object sourceT1 = sourceWithTransformT1();
    return transformT2(sourceT1);
  }

  static Object sourceWithoutTransformUnused() {
    // Expect inferred generation:
    //   Return -> Source
    return transformUnused(Origin.source());
  }

  static void sinkWithTransformT1(Object o) {
    // Expect inferred sinks:
    //   Argument(0) -> `T1@Sink`
    //               -> `Sink`
    Object t1 = transformT1(o);
    Origin.sink(t1);
  }

  static void sinkWithTransformT2(Object o) {
    // Expect inferred sinks:
    //   Argument(0) -> `T2@Sink`
    Object t2 = transformT2(o);
    Origin.sink(t2);
  }

  static void sinkWithTransformsT1T2(Object o) {
    // Expect inferred sinks:
    //   Argument(0) -> `T1:T2@Sink`
    //               -> `T2@Sink`
    Object sourceT1 = transformT1(o);
    Object sourceT2 = transformT2(sourceT1);
    Origin.sink(sourceT2);
  }

  static void hopSinkWithTransformsT1T2(Object o) {
    // Expect inferred sinks:
    //   Argument(0) -> `T1@T2:Sink`
    //               -> `T2:Sink`
    Object t = transformT1(o);
    sinkWithTransformT2(t);
  }

  static void sinkWithoutTransformUnused(Object o) {
    // Expect inferred sinks:
    //   Argument(0) -> `Sink`
    Origin.sink(transformUnused(o));
  }

  // -------------------------------------------------------
  //  Issues
  // -------------------------------------------------------
  public static void testSourceWithTransformIssue() {
    // Expect issue for rules:
    //   Source -> T1 -> Sink
    //   Source -> Sink
    Origin.sink(sourceWithTransformT1());
  }

  public static void testSinkWithTransformIssue() {
    // Expect issue for rules:
    //   Source -> T1 -> Sink
    //   Source -> Sink
    sinkWithTransformT1(Origin.source());
  }

  public static void testSourceSinkWithTransformSafe() {
    // Expect No issue as No rules for:
    //   NewSource -> T2 -> Sink
    //   NewSource -> T2 -> T1 -> Sink
    Object sourceWithT2 = transformT2(getNewSource());
    sinkWithTransformT1(sourceWithT2);
  }

  public static void testIssuePropagateWithSource() {
    Data data = new Data();
    data.foo = Origin.source();
    data = propagateWithTransformT3WithSource(data);

    // Expect issue for rule:
    //   Source -> T3 -> Sink
    //   NewSource -> Sink
    Origin.sink(data);
  }

  public static void testPropagateAlternateTransforms() {
    Data data = new Data();
    data.foo = Origin.source();
    data.bar = getNewSource();
    data = propagateAlternateTransforms(data);

    // Expect issue for rules:
    //   Source -> Sink
    //   Source -> T1 -> Sink
    Origin.sink(data);
  }

  public static void testMultihopSource1() {
    // Expect issue for rules:
    //   Source -> T2 -> Sink
    //   Source -> T1 -> T2 -> Sink
    Object source = sourceWithTransformsT1T2();
    Origin.sink(source);
  }

  public static void testMultihopSource2() {
    // Expect issue for rules:
    //   Source -> T2 -> Sink
    //   Source -> T1 -> T2 -> Sink
    Object source = hopSourceWithTransformsT1T2();
    Origin.sink(source);
  }

  public static void testMultihopSink1() {
    // Expect issue for rules:
    //   Source -> T2 -> Sink
    //   Source -> T1 -> T2 -> Sink
    Object source = Origin.source();
    sinkWithTransformsT1T2(source);
  }

  public static void testMultihopSink2() {
    // Expect issue for rules:
    //   Source -> T2 -> Sink
    //   Source -> T1 -> T2 -> Sink
    Object source = Origin.source();
    hopSinkWithTransformsT1T2(source);
  }

  public static void testMultihopSourceSink() {
    // Expect issue for rules:
    //   Source -> T2 -> Sink
    //   Source -> T1 -> T2 -> Sink
    Object source = sourceWithTransformT1();
    sinkWithTransformT2(source);
  }

  public static void testTransformsT1T2T1T2Issue() {
    // Expect issue for rules:
    //   Source -> T1 -> T2 -> T1 -> T2 -> Sink
    Object source = hopSourceWithTransformsT1T2();
    hopSinkWithTransformsT1T2(source);
  }

  public static void testForAllParameters() {
    // Expect issue for rules:
    //   Source -> T1 -> T2 -> Sink
    Object source = transformForAllParameters(new Data(), Origin.source());
    sinkWithTransformT2(source);
  }

  public static void testUnusedTransforms() {
    // Unused transform is not part of any rule, so is not applied.
    // Only returns Source.
    Object source = sourceWithoutTransformUnused();
    Object stillJustSource = transformUnused(source);
    // Unused is excluded, but T1 is still applied.
    Object sourceT1 = propagateWithTransformsT1Unused(stillJustSource);
    // Unused is excluded, but Sink still exists.
    // Expect issue for rules:
    //   Source -> Sink
    //   Source -> T1 -> Sink
    sinkWithoutTransformUnused(sourceT1);
  }

  public static void testTransformT2T3Issue() {
    // Expect issue for rules:
    //   Source -> T2 -> T3 -> Sink
    Object source = Origin.source();
    Object sourceT2 = transformT2(source);
    Object sourceT2T3 = transformT3(sourceT2);
    Origin.sink(sourceT2T3);
  }

  public static void testPropagateWithTransformT2T3Issue() {
    // Expect issue for rules:
    //   Source -> T2 -> T3 -> Sink
    Object source = Origin.source();
    Object sourceT2T3 = propagateWithTransformT2T3(source);
    Origin.sink(sourceT2T3);
  }
}
