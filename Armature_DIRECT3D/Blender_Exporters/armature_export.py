import bpy

#f = open("YOUR_OUTPUT_PATH", "w")

action_groups = bpy.context.object.animation_data.action.groups

num_bones = len(bpy.context.scene.objects.active.pose.bones)

f.write("NUM_BONES: " + str(num_bones) + "\n\n")

for bone in bpy.context.scene.objects.active.pose.bones:
    #nazwa kosci
    f.write("////////BONE_ID: " + bone.name +"//////////\n")
    
    #rodzic
    p = bone.parent
    if p == None:
        f.write("Parent_ID: Root\n")
    else:
        f.write("Parent_ID: " + bone.parent.name +"\n")
    
    #rozmiar
    len_vec = bone.tail - bone.head
    f.write("Size: " + str(len_vec.length) + "\n")
    q = bone.bone.matrix_local.to_quaternion()
    f.write("Quaternion local: " +  str(q.w) + " " + str(q.x) + " " + str(q.y) + " "+ str(q.z) +"\n")
    pos = bone.bone.matrix_local.translation
    f.write("Location local: " + str(pos.x) + " " + str(pos.y) + " "+ str(pos.z) +"\n")
    q = bone.matrix_basis.to_quaternion()
    f.write("Quaternion basis: " +  str(q.w) + " " + str(q.x) + " " + str(q.y) + " "+ str(q.z) +"\n")
    pos = bone.matrix_basis.translation
    f.write("Location basis: " + str(pos.x) + " " + str(pos.y) + " "+ str(pos.z) +"\n")

    group = action_groups.get(bone.name)    

    coord_names = ["w", "x", "y", "z"]
    
    if(group != None):    
        for channel in group.channels:
            if "rotation_quaternion" in channel.data_path:
                f.write(str(coord_names[channel.array_index]) + " :\n")
                for keyframe in channel.keyframe_points:
                    f.write(str(keyframe.co[0])+ ", " +str(keyframe.co[1])+ "\n")
        
    f.write("\n")



f.close()

print("bla bla")