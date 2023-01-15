import bpy
object = bpy.data.objects["OBJECT_NAME"]


vert_list = object.data.vertices;
vert_groups = object.vertex_groups


#f = open("YOUR_OUTPUT_PATH","w")

for g in vert_groups:
    print(g.name)

v_index = 1
for v in vert_list:
    f.write("vertex: " + str(v_index) +"\n")
    f.write(str(v.co[0]) +" "+ str(v.co[1])+" "+ str(v.co[2]) +"\n")
    for g in v.groups:
        for g2 in vert_groups:
            if(g.group == g2.index):
                f.write("   " + str(g2.name) + " " + str(g.weight) +"\n")
        
    v_index+=1

f.close()