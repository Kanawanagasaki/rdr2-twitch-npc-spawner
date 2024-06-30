using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace Rdr2.TwitchNpcSpawner.Migrations
{
    /// <inheritdoc />
    public partial class AddExtraFieldToReward : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<string>(
                name: "Extra",
                table: "rewards",
                type: "longtext",
                nullable: true)
                .Annotation("MySql:CharSet", "utf8mb4");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "Extra",
                table: "rewards");
        }
    }
}
