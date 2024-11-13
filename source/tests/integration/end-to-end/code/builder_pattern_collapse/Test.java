/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.marianatrench.integrationtests;

public class Test {
  // 4 fields, Heuristics configured with max sizes for generations/sinks/propagations less than 4.
  // Check heuristics.json
  public Test a;
  public Test b;
  public Test c;
  public Test d;

  public Test() {}

  public Test(Test a, Test b, Test c, Test d) {
    this.a = a;
    this.b = b;
    this.c = c;
    this.d = d;
  }

  public Test(Test.Builder builder) {
    // Global config override with max width = 4
    this.a = builder.a;
    this.b = builder.b;
    this.c = builder.c;
    this.d = builder.d;
  }

  private static Test withSources() {
    // Global config override with max width = 3
    Test result = new Test();
    result.a = (Test) Origin.source();
    result.b = (Test) Test.differentSource();
    result.c = (Test) Origin.source();

    return result;
  }

  public static void sinkFields(Test test) {
    // Global config override with max width = 3
    Origin.sink(test.a);
    Test.differentSink(test.b);
    Origin.sink(test.c);
  }

  private static Object differentSource() {
    return new Object();
  }

  private static void differentSink(Object obj) {}

  public void doNotInlineAsSetter(Test other) {
    this.a.b.c = other.c.d.a;
  }

  public Test doNotInlineAsGetter() {
    return this.a.b.c;
  }

  public static final class Builder {
    public Test a;
    public Test b;
    public Test c;
    public Test d;

    public Builder setA(Test a) {
      this.a = a;
      return this;
    }

    public Builder setB(Test b) {
      this.b = b;
      return this;
    }

    public Builder setC(Test c) {
      this.c = c;
      return this;
    }

    public Builder setD(Test d) {
      this.d = d;
      return this;
    }

    public Test buildUsingFieldsAsArguments() {
      // Currently, the heuristics set the propagation input/output path sizes to 2.
      // This means we infer a widen-broadened propagation of: Argument(0) -> LocalReturn
      return new Test(this.a, this.b, this.c, this.d);
    }

    public Test buildUsingBuilderAsArgument() {
      // Expect config override with max width = 4 propagated from the Test(Builder) constructor.
      return new Test(this);
    }
  }

  public static Test collapseOnBuilder() {
    // Resulting taint tree will have caller port Return
    // Taint 3 fields to trigger collapsing behavior,
    // configure generation_max_output_path_leaves = 2
    return (new Builder())
        .setA((Test) Origin.source())
        .setB((Test) Test.differentSource())
        .setC((Test) Origin.source())
        .buildUsingFieldsAsArguments();
  }

  public static Test noCollapseOnBuilder() {
    // Resulting taint tree will have caller port Return.a and Return.b
    return (new Builder())
        .setA((Test) Origin.source())
        .setB((Test) Test.differentSource())
        .buildUsingFieldsAsArguments();
  }

  public static Test noCollapseOnApproximateBuildUsingFields() {
    // Still the 3-field pattern, but with "no-collapse-on-approximate" enabled.
    // But currently, the heuristics set the propagation input/output path sizes to 2, which means
    // that propagation through buildUsingFieldsAsArguments() is already collapsed and
    // over-approximates. Hence, we assign the collapsed taint to precise paths .a, .b, .c. So even
    // though we do not collapse these paths because of the "no-collapse-on-approximate" mode, we
    // still get false positive results in the testNoCollapseOnApproximateBuildUsingFields().
    return (new Builder())
        .setA((Test) Origin.source())
        .setB((Test) Test.differentSource())
        .setC((Test) Origin.source())
        .buildUsingFieldsAsArguments();
  }

  public static Test noCollapseOnApproximateBuildUsingThis() {
    // Still the 3-field pattern, but with "no-collapse-on-approximate" enabled.
    return (new Builder())
        .setA((Test) Origin.source())
        .setB((Test) Test.differentSource())
        .setC((Test) Origin.source())
        .buildUsingBuilderAsArgument();
  }

  public static Test buildUsingThis() {
    // Still the 3-field pattern but **without** "no-collapse-on-approximate". Since this uses the
    // Test(Builder) constructor with the max width config override of 4, we no longer need to
    // specify the no-collapse-on-approximate mode on the wrapper methods.
    // Expect the same result as noCollapseOnApproximateBuildUsingThis.
    return (new Builder())
        .setA((Test) Origin.source())
        .setB((Test) Test.differentSource())
        .setC((Test) Origin.source())
        .buildUsingBuilderAsArgument();
  }

  public static Test buildWithSources() {
    // withSources() has max width config overrides of 3. We should infer generations with the
    // same config overrides.
    return Test.withSources();
  }

  public static void toSinkFields(Test test) {
    Test.sinkFields(test);
  }

  public static void testCollapseOnBuilder() {
    Test output = collapseOnBuilder();

    // issue: a has "Source" and b has "DifferentSource"
    Origin.sink(output.a);
    Test.differentSink(output.b);

    // Also issue: false positive due to collapsing
    Test.differentSink(output.a);
    Origin.sink(output.b);
    Origin.sink(output.d);
    Test.differentSink(output.d);
  }

  public static void testNoCollapseOnBuilder() {
    Test output = noCollapseOnBuilder();

    // issue: a has "Source" and b has "DifferentSource"
    Origin.sink(output.a);
    Test.differentSink(output.b);

    // no issue, since no collapsing occurs
    Test.differentSink(output.a);
    Origin.sink(output.b);
    Origin.sink(output.d);
    Test.differentSink(output.d);
  }

  public static void testNoCollapseOnApproximateBuildUsingFields() {
    Test output = noCollapseOnApproximateBuildUsingFields();

    // issue: a has "Source" and b has "DifferentSource"
    Origin.sink(output.a);
    Test.differentSink(output.b);

    // no issue, since no collapsing occurs
    // Currently FP. See noCollapseOnApproximateBuildUsingFields()
    Test.differentSink(output.a);
    Origin.sink(output.b);
    Origin.sink(output.d);
    Test.differentSink(output.d);
  }

  public static void testNoCollapseOnApproximiateBuildUsingThis() {
    Test output = noCollapseOnApproximateBuildUsingThis();

    // issue: a has "Source" and b has "DifferentSource"
    Origin.sink(output.a);
    Test.differentSink(output.b);

    // no issue, since no collapsing occurs
    Test.differentSink(output.a);
    Origin.sink(output.b);
    Origin.sink(output.d);
    Test.differentSink(output.d);
  }

  public static void testBuildUsingThis() {
    Test output = buildUsingThis();

    // issue: a has "Source" and b has "DifferentSource"
    Origin.sink(output.a);
    Test.differentSink(output.b);

    // Expect no issue, since no collapsing occurs.
    Test.differentSink(output.a);
    Origin.sink(output.b);
    Origin.sink(output.d);
    Test.differentSink(output.d);
  }

  public void testInlineAsGetter() {
    Test b = new Test();
    b.c = (Test) Origin.source(); // b.c = tainted

    Test a = new Test();
    a.b = b; // a.b.c = tainted

    Test x = new Test();
    x.a = a; // x.a.b.c = tainted

    // Expect issue as: x.a.b.c is tainted so x.a.b.c.d is also tainted.
    Origin.sink(x.a.b.c.d);
    // Expect issue with propagation_max_input_path_size = 2.
    // - inline_as_getter: Argument(0).a.b.c
    // - propagation: Argument(0).a.b -> LocalReturn (collapse depth = 0)
    // - is_safe_to_inline is false as inline_as_getter and propagation mismatch.
    // - so propagation model is applied.
    // - Since collapse depth is 0, result memory location after call to doNotInlineAsGetter() is
    //   tainted and we find the issue.
    Origin.sink(x.doNotInlineAsGetter().d);
  }

  public void testInlineAsSetter() {
    Test b = new Test();
    b.c.d.a = (Test) Origin.source(); // b.c.d.a = tainted

    // Expect no issues as: only b.c.d.a is tainted
    Origin.sink(b.c.d.b);

    Test x = new Test();
    x.doNotInlineAsSetter(b); // x.a.b.c = tainted
    // Should be no issue as: only x.a.b.c is tainted
    // But FP when propagation_max_[input|output]_path_size = 2 which is the expected sound result.
    // For doNotInlineAsSetter():
    // - inline_as_setter: value=Argument(1).c.d.a, target=Argument(0).a.b.c
    // - propagation: Argument(1).c.d -> Argument(0).a.b (collapse depth = 0)
    // - is_safe_to_inline is false as inline_as_setter and propagation mismatch.
    // - so propagation model is applied.
    // - result memory location after call to doNotInlineAsSetter() is: .a.b = tainted
    // FP issue (as expected) as x.a.b is tainted so x.a.b.d is also tainted.
    Origin.sink(x.a.b.d);
  }

  public void testbuildWithSources() {
    Test output = buildWithSources();

    // issue: a has "Source" and b has "DifferentSource"
    Origin.sink(output.a);
    Test.differentSink(output.b);

    // Expect no issue, since no collapsing occurs.
    Test.differentSink(output.a);
    Origin.sink(output.b);
    Test.differentSink(output.c);
    Origin.sink(output.d);
    Test.differentSink(output.d);
  }

  public void testToSinkFields() {
    Test output = new Test();
    output.a = (Test) Origin.source();
    output.b = (Test) Test.differentSource();
    output.c = (Test) Origin.source();
    output.d = (Test) Test.differentSource();

    // Expect:
    // .a: Source -> Sink issue
    // .b: DifferentSource -> DifferentSink issue
    // .c: Source -> Sink issue
    // .d: No issue
    toSinkFields(output);
  }
}
