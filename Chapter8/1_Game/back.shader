; renderer for the tetris field

Object("clRenderState")
{
   Images()
   { 
     Texture 0 "back.png"
     Texture 1 "back_high_top.png"
     Texture 2 "back_high_bottom.png"
   }

   DepthTest False

   ShaderProgram "back.sp"
}
