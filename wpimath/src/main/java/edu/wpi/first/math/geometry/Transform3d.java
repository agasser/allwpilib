// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

package edu.wpi.first.math.geometry;

import edu.wpi.first.math.proto.Geometry3D.ProtobufTransform3d;
import edu.wpi.first.util.protobuf.Protobuf;
import edu.wpi.first.util.struct.Struct;
import java.nio.ByteBuffer;
import java.util.Objects;
import us.hebi.quickbuf.Descriptors.Descriptor;

/** Represents a transformation for a Pose3d in the pose's frame. */
public class Transform3d {
  private final Translation3d m_translation;
  private final Rotation3d m_rotation;

  /**
   * Constructs the transform that maps the initial pose to the final pose.
   *
   * @param initial The initial pose for the transformation.
   * @param last The final pose for the transformation.
   */
  public Transform3d(Pose3d initial, Pose3d last) {
    // We are rotating the difference between the translations
    // using a clockwise rotation matrix. This transforms the global
    // delta into a local delta (relative to the initial pose).
    m_translation =
        last.getTranslation()
            .minus(initial.getTranslation())
            .rotateBy(initial.getRotation().unaryMinus());

    m_rotation = last.getRotation().minus(initial.getRotation());
  }

  /**
   * Constructs a transform with the given translation and rotation components.
   *
   * @param translation Translational component of the transform.
   * @param rotation Rotational component of the transform.
   */
  public Transform3d(Translation3d translation, Rotation3d rotation) {
    m_translation = translation;
    m_rotation = rotation;
  }

  /**
   * Constructs a transform with x, y, and z translations instead of a separate Translation3d.
   *
   * @param x The x component of the translational component of the transform.
   * @param y The y component of the translational component of the transform.
   * @param z The z component of the translational component of the transform.
   * @param rotation The rotational component of the transform.
   */
  public Transform3d(double x, double y, double z, Rotation3d rotation) {
    m_translation = new Translation3d(x, y, z);
    m_rotation = rotation;
  }

  /** Constructs the identity transform -- maps an initial pose to itself. */
  public Transform3d() {
    m_translation = new Translation3d();
    m_rotation = new Rotation3d();
  }

  /**
   * Multiplies the transform by the scalar.
   *
   * @param scalar The scalar.
   * @return The scaled Transform3d.
   */
  public Transform3d times(double scalar) {
    return new Transform3d(m_translation.times(scalar), m_rotation.times(scalar));
  }

  /**
   * Divides the transform by the scalar.
   *
   * @param scalar The scalar.
   * @return The scaled Transform3d.
   */
  public Transform3d div(double scalar) {
    return times(1.0 / scalar);
  }

  /**
   * Composes two transformations. The second transform is applied relative to the orientation of
   * the first.
   *
   * @param other The transform to compose with this one.
   * @return The composition of the two transformations.
   */
  public Transform3d plus(Transform3d other) {
    return new Transform3d(new Pose3d(), new Pose3d().transformBy(this).transformBy(other));
  }

  /**
   * Returns the translation component of the transformation.
   *
   * @return The translational component of the transform.
   */
  public Translation3d getTranslation() {
    return m_translation;
  }

  /**
   * Returns the X component of the transformation's translation.
   *
   * @return The x component of the transformation's translation.
   */
  public double getX() {
    return m_translation.getX();
  }

  /**
   * Returns the Y component of the transformation's translation.
   *
   * @return The y component of the transformation's translation.
   */
  public double getY() {
    return m_translation.getY();
  }

  /**
   * Returns the Z component of the transformation's translation.
   *
   * @return The z component of the transformation's translation.
   */
  public double getZ() {
    return m_translation.getZ();
  }

  /**
   * Returns the rotational component of the transformation.
   *
   * @return Reference to the rotational component of the transform.
   */
  public Rotation3d getRotation() {
    return m_rotation;
  }

  /**
   * Invert the transformation. This is useful for undoing a transformation.
   *
   * @return The inverted transformation.
   */
  public Transform3d inverse() {
    // We are rotating the difference between the translations
    // using a clockwise rotation matrix. This transforms the global
    // delta into a local delta (relative to the initial pose).
    return new Transform3d(
        getTranslation().unaryMinus().rotateBy(getRotation().unaryMinus()),
        getRotation().unaryMinus());
  }

  @Override
  public String toString() {
    return String.format("Transform3d(%s, %s)", m_translation, m_rotation);
  }

  /**
   * Checks equality between this Transform3d and another object.
   *
   * @param obj The other object.
   * @return Whether the two objects are equal or not.
   */
  @Override
  public boolean equals(Object obj) {
    if (obj instanceof Transform3d) {
      return ((Transform3d) obj).m_translation.equals(m_translation)
          && ((Transform3d) obj).m_rotation.equals(m_rotation);
    }
    return false;
  }

  @Override
  public int hashCode() {
    return Objects.hash(m_translation, m_rotation);
  }

  public static final class AStruct implements Struct<Transform3d> {
    @Override
    public Class<Transform3d> getTypeClass() {
      return Transform3d.class;
    }

    @Override
    public String getTypeString() {
      return "struct:Transform3d";
    }

    @Override
    public int getSize() {
      return Translation3d.struct.getSize() + Rotation3d.struct.getSize();
    }

    @Override
    public String getSchema() {
      return "Translation3d translation;Rotation3d rotation";
    }

    @Override
    public Struct<?>[] getNested() {
      return new Struct<?>[] {Translation3d.struct, Rotation3d.struct};
    }

    @Override
    public Transform3d unpack(ByteBuffer bb) {
      Translation3d translation = Translation3d.struct.unpack(bb);
      Rotation3d rotation = Rotation3d.struct.unpack(bb);
      return new Transform3d(translation, rotation);
    }

    @Override
    public void pack(ByteBuffer bb, Transform3d value) {
      Translation3d.struct.pack(bb, value.m_translation);
      Rotation3d.struct.pack(bb, value.m_rotation);
    }
  }

  public static final AStruct struct = new AStruct();

  public static final class AProto implements Protobuf<Transform3d, ProtobufTransform3d> {
    @Override
    public Class<Transform3d> getTypeClass() {
      return Transform3d.class;
    }

    @Override
    public Descriptor getDescriptor() {
      return ProtobufTransform3d.getDescriptor();
    }

    @Override
    public Protobuf<?, ?>[] getNested() {
      return new Protobuf<?, ?>[] {Translation3d.proto, Rotation3d.proto};
    }

    @Override
    public ProtobufTransform3d createMessage() {
      return ProtobufTransform3d.newInstance();
    }

    @Override
    public Transform3d unpack(ProtobufTransform3d msg) {
      return new Transform3d(
          Translation3d.proto.unpack(msg.getTranslation()),
          Rotation3d.proto.unpack(msg.getRotation()));
    }

    @Override
    public void pack(ProtobufTransform3d msg, Transform3d value) {
      Translation3d.proto.pack(msg.getMutableTranslation(), value.m_translation);
      Rotation3d.proto.pack(msg.getMutableRotation(), value.m_rotation);
    }
  }

  public static final AProto proto = new AProto();
}
